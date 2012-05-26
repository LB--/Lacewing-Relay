
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../Common.h"

#define Callback(F) \
    static int F (http_parser * parser) \
    {   return ((HTTPClient *) parser->data)->F (); \
    }

/* This discards the const qualifier, because it'll be part of one
 * of our (non-const) buffers anyway. */

#define DataCallback(F) \
    static int F (http_parser * parser, const char * buf, size_t len) \
    {   return ((HTTPClient *) parser->data)->F ((char *) buf, len); \
    }

 Callback (onMessageBegin)
 Callback (onHeadersComplete)
 Callback (onMessageComplete)

 DataCallback (onURL)
 DataCallback (onBody)
 DataCallback (onHeaderField)
 DataCallback (onHeaderValue)

#undef Callback
#undef DataCallback

static http_parser_settings ParserSettings = { 0 };

HTTPClient::HTTPClient (Webserver::Internal &_Server, Server::Client &_Socket, bool Secure)
    : Request (_Server, *this), WebserverClient (_Server, _Socket, Secure)
{
    Multipart = 0;
    Request.Clean ();

    http_parser_init (&Parser, HTTP_REQUEST);
    Parser.data = this;

    ParsingHeaders = true;
    SignalEOF = false;
}

HTTPClient::~HTTPClient ()
{
    delete Multipart;
}

void HTTPClient::Process (char * buffer, size_t size)
{
    if (!size)
        return;

    if (!ParserSettings.on_message_begin)
    {
        ParserSettings.on_message_begin      = ::onMessageBegin;
        ParserSettings.on_url                = ::onURL;
        ParserSettings.on_header_field       = ::onHeaderField;
        ParserSettings.on_header_value       = ::onHeaderValue;
        ParserSettings.on_headers_complete   = ::onHeadersComplete;
        ParserSettings.on_body               = ::onBody;
        ParserSettings.on_message_complete   = ::onMessageComplete;
    }

    /* TODO: A naughty client could keep the connection open by sending 1 byte every 5 seconds */

    LastActivityTime = time (0);

    if (!Request.Responded)
    {
        /* The application hasn't yet called Finish() for the last request, so this data
           can't be processed.  Buffer it to process when Finish() is called. */

        this->Buffer.Add (buffer, size);
        return;
    }

    if (ParsingHeaders)
    {
        for (int i = 0; i < size; )
        {
            {   char b = buffer [i];

                if (b == '\r')
                {
                    if (buffer [i + 1] == '\n')
                        ++ i;
                }
                else if (b != '\n')
                {
                    ++ i;
                    continue;
                }
            }

            int toParse = i + 1;
            bool error = false;

            if (Buffer.Size)
            {
                Buffer.Add (buffer, toParse);

                size_t parsed = http_parser_execute
                    (&Parser, &ParserSettings, Buffer.Buffer, Buffer.Size);

                if (parsed != Buffer.Size)
                    error = true;

                Buffer.Reset ();
            }
            else
            {
                size_t parsed = http_parser_execute
                    (&Parser, &ParserSettings, buffer, toParse);

                if (parsed != toParse)
                    error = true;
            }

            size -= toParse;
            buffer += toParse;

            if (error || Parser.upgrade)
            {
                DebugOut ("HTTP error, closing socket...");

                Socket.Close ();
                return;
            }

            i = 0;
        }

        if (ParsingHeaders)
        {
            /* TODO : max line length */

            this->Buffer.Add (buffer, size);
            return;
        }
    }

    int parsed = http_parser_execute (&Parser, &ParserSettings, buffer, size);

    if (SignalEOF)
    {
        http_parser_execute (&Parser, &ParserSettings, 0, 0);
        SignalEOF = false;
    }

    if (parsed != size || Parser.upgrade)
    {
        Socket.Close ();
        return;
    }
}

int HTTPClient::onMessageBegin ()
{
    Request.Clean ();

    return 0;
}

int HTTPClient::onURL (char * URL, size_t length)
{
    /* Writing a null terminator to the end and then restoring it afterwards.
     * This assumes the byte after the URL is writable (it should be a space)
     */

    char * end = URL + length, b = *end;

    LacewingAssert (b == ' ');

    *end = 0;
    bool success = Request.In_URL (URL);
    *end = b;

    return success ? 0 : -1;
}

int HTTPClient::onHeaderField (char * buffer, size_t length)
{
    /* Since we already ensure the parser receives entire lines while processing
     * headers, it's safe to just save the pointer and size from onHeaderField
     * and use them in onHeaderValue.
     */

    CurHeaderName = buffer;
    CurHeaderNameLength = length;

    return 0;
}

int HTTPClient::onHeaderValue (char * value, size_t length)
{
    /* The parser never reads back, so might as well destroy the byte following
     * the header name (it can't be anything to do with the value, because
     * there has to be a `:` in the middle)
     */

    CurHeaderName [CurHeaderNameLength] = 0;

    char * end = value + length, b = *end;

    LacewingAssert (b == '\r' || b == '\n');

    *end = 0;
    Request.In_Header (CurHeaderName, value);
    *end = b;

    return 0;
}

int HTTPClient::onHeadersComplete ()
{
    ParsingHeaders = false;

    Request.In_Method
        (http_method_str ((http_method) Parser.method));

    {   const char * ContentType = Request.InHeaders.Get ("Content-Type");

        if (BeginsWith (ContentType, "multipart"))
            Multipart = new MultipartProcessor (*this, ContentType);
    }

    return 0;
}

int HTTPClient::onBody (char * buffer, size_t size)
{
    if (!Multipart)
    {
        /* Normal request body - just buffer it */

        this->Buffer.Add (buffer, size);
        return 0;
    }

    /* Multipart request body - hand it over to Multipart */

    Multipart->Process (buffer, size);
 
    if (Multipart->State == MultipartProcessor::Error)
    {
        delete Multipart;
        Multipart = 0;

        return -1;
    }

    if (Multipart->State == MultipartProcessor::Done)
    {
        Multipart->CallRequestHandler ();
 
        delete Multipart;
        Multipart = 0;

        SignalEOF = true;
    }

    return 0;
}

int HTTPClient::onMessageComplete ()
{
    if (this->Buffer.Size)
    {
        this->Buffer.Add <char> (0);

        char * post_data = this->Buffer.Buffer,
                 * end = post_data + this->Buffer.Size, b = *end;

        *end = 0;

        for (;;)
        {
            char * name = post_data, * value = strchr (post_data, '=');

            if (!value ++)
                break;

            char * next = strchr (value, '&');

            int name_length = (value - name) - 1,
                value_length = next ? next - value : strlen (value);

            char * name_decoded = (char *) malloc (name_length + 1),
                 * value_decoded = (char *) malloc (value_length + 1);

            if(!URLDecode (name, name_length, name_decoded, name_length + 1)
                    || !URLDecode (value, value_length, value_decoded, value_length + 1))
            {
                free (name_decoded);
                free (value_decoded);
            }
            else
                Request.PostItems.Set (name_decoded, value_decoded, false);

            if(!(post_data = next))
                break;
        }

        *end = b;

        this->Buffer.Reset();
    }

    Request.RunStandardHandler ();

    ParsingHeaders = true;

    return 0;
}

void HTTPClient::Respond (Webserver::Request::Internal &) /* request parameter ignored - HTTP only ever has one request object per client */
{
    LastActivityTime = time (0);

    Buffer.Reset ();

    Buffer << "HTTP/" << Parser.http_major << "." << Parser.http_minor << " " << Request.Status;
    
    for(Map::Item * Current = Request.OutHeaders.First; Current; Current = Current->Next)
        Buffer << "\r\n" << Current->Key << ": " << Current->Value;

    for(Map::Item * Current = Request.OutCookies.First; Current; Current = Current->Next)
    {
        const char * OldValue = Request.InCookies.Get(Current->Key);
        
        int ValueSize = (int) (strchr (Current->Value, ';') - Current->Value);

        if (ValueSize < 0)
            ValueSize = strlen (Current->Value);

        if (ValueSize == strlen (OldValue) && memcmp (OldValue, Current->Value, ValueSize) == 0)
            continue;

        Buffer << "\r\nSet-Cookie: " << Current->Key << "=" << Current->Value;
    }

    Buffer << "\r\nContent-Length: " << Request.Public.Queued ();
    Buffer << "\r\n\r\n";

    Socket.Cork ();

    {   Stream::HeadBuffer head [1];
        
        head [0].Buffer = Buffer.Buffer;
        head [0].Length = Buffer.Size;

        Request.Public.EndQueue (1, head);
    }

    Request.Public.BeginQueue ();

    Buffer.Reset ();
    Socket.Uncork ();

    if (!http_should_keep_alive (&Parser))
        Socket.Close ();
}

void HTTPClient::Dead ()
{
    if (Request.Responded)
    {
        /* The request isn't unfinished - don't call the disconnect handler */
        return;
    }

    if (Server.Handlers.Disconnect)
        Server.Handlers.Disconnect (Server.Webserver, Request.Public);
}

bool HTTPClient::IsSPDY ()
{
    return false;
}

void HTTPClient::Tick ()
{
    if (Request.Responded && (time(0) - LastActivityTime) > Timeout)
    {
        DebugOut ("Dropping HTTP connection due to inactivity (%s/%d)", Socket.GetAddress().ToString(), &Socket.GetAddress());
        Socket.Close ();
    }
}

