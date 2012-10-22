
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
    Request.Clean ();

    http_parser_init (&Parser, HTTP_REQUEST);
    Parser.data = this;

    ParsingHeaders = true;
    SignalEOF = false;

    _Socket.Write (Request);
    Request.BeginQueue ();

    /* When the retry mode is Retry_MoreData and Put can't consume everything,
     * Put will be called again as soon as more data arrives.
     */

    Retry (Retry_MoreData);
}

HTTPClient::~HTTPClient ()
{
    /* Only call the disconnect handler for requests that have not yet been
     * completed (Responded == false)
     */

    if (!Request.Responded)
    {
        if (Server.Handlers.Disconnect)
            Server.Handlers.Disconnect (Server.Webserver, Request);
    }
}

size_t HTTPClient::Put (const char * buffer, size_t size)
{
    lwp_trace ("HTTP got " lwp_fmt_size " bytes", size);
    
    size_t processed = 0;

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

    /* TODO: A naughty client could keep the connection open by sending 1 byte
     * every 5 seconds.
     */

    LastActivityTime = time (0);

    for (;;)
    {
        if (!Request.Responded)
        {
            /* The application hasn't yet called Finish() for the last request,
             * so no more data can be processed.
             */

            return processed;
        }

        /* Already processed everything in the last loop iteration? */

        if (processed == size)
            return processed;

        /* When parsing headers, we provide the HTTP parser with complete lines
         * to avoid getting any fragmented data.
         *
         * When partial lines are received, we can take advantage of the
         * natural buffering provided by HTTPClient being a stream (by
         * returning < size bytes from Put and calling Retry later).
         */

        /* TODO: max line length */

        if (ParsingHeaders)
        {
            for (size_t i = processed; i < size; ++ i)
            {
                char b = buffer [i];

                if (b == '\r' || b == '\n')
                {
                    /* Reached the end of a line */

                    size_t toParse = (i + 1) - processed;

                    size_t parsed = http_parser_execute
                        (&Parser, &ParserSettings, buffer + processed, toParse);

                    processed += parsed;

                    if (Parser.http_errno == HPE_PAUSED)
                    {
                        /* Paused by onMessageEnd - this has no significance, since
                         * we're already parsing headers.
                         */

                        http_parser_pause (&Parser, 0);
                        continue;
                    }
                    else if (parsed != toParse || Parser.upgrade)
                    {
                        lwp_trace ("HTTP error (headers), closing socket...");

                        Socket.Close ();
                        return size;
                    }

                    if (!ParsingHeaders)
                        break;  /* headers finished */
                }
            }

            /* Reached the end of the buffer - are we still parsing headers? */

            if (ParsingHeaders)
                return processed;
            else
            {
                if (processed == size)
                    return processed;
            }
        }

        /* Parsing the body.
         *
         * Note that we have no idea if the buffer we have here contains only
         * body data - it might well contain a fragment of the next request,
         * which we would want to pass to the HTTP parser line by line.  For
         * this reason, the parser will always be paused in onMessageComplete
         * so that control is returned here.
         */

        size_t parsed =
            http_parser_execute (&Parser, &ParserSettings,
                                   buffer + processed, size - processed);

        processed += parsed;

        if (Parser.http_errno == HPE_PAUSED)
        {
            /* onMessageComplete always pauses the parser so that control
             * is returned here, so that we can transition back to parsing
             * headers.
             */

            http_parser_pause (&Parser, 0);
        }
        else if (parsed != size || Parser.upgrade)
        {
            lwp_trace ("HTTP error (body), closing socket...");

            Socket.Close ();
            return size;
        }

        if (SignalEOF)
        {
            http_parser_execute (&Parser, &ParserSettings, 0, 0);
            SignalEOF = false;
        }

        return processed;
    }
}

int HTTPClient::onMessageBegin ()
{
    Request.Clean ();

    return 0;
}

int HTTPClient::onURL (char * URL, size_t length)
{
    if (!Request.In_URL (length, URL))
    {
        lwp_trace ("HTTP: Bad URL");
        return -1;
    }

    return 0;
}

int HTTPClient::onHeaderField (char * buffer, size_t length)
{
    if (!Request.Version_Major)
    {
        char version [16];

        lwp_snprintf (version, sizeof (version), "HTTP/%d.%d",
                (int) Parser.http_major, (int) Parser.http_minor);

        if (!Request.In_Version (strlen (version), version))
        {
            lwp_trace ("HTTP: Bad version");
            return -1;
        }
    }

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
    if (!Request.In_Header (CurHeaderNameLength, CurHeaderName, length, value))
    {
        lwp_trace ("HTTP: Bad header");
        return -1;
    }

    return 0;
}

int HTTPClient::onHeadersComplete ()
{
    ParsingHeaders = false;

    const char * method = http_method_str ((http_method) Parser.method);
    
    if (!Request.In_Method (strlen (method), method))
    {
        lwp_trace ("HTTP: Bad method");
        return -1;
    }

    const char * content_type = Request.Header ("Content-Type");

    lwp_trace ("Content-Type is %s", content_type);

    if (lwp_begins_with (content_type, "multipart"))
    {
        lwp_trace ("Creating Multipart...");

        if (! (Multipart = new (std::nothrow)
                    struct Multipart (Server, Request, content_type)))
        {
            return -1;
        }
    }

    return 0;
}

int HTTPClient::onBody (char * buffer, size_t size)
{
    if (!Multipart)
    {
        /* Normal request body - just buffer it */

        Request.Buffer.Add (buffer, size);
        return 0;
    }

    /* Multipart request body - hand it over to Multipart */

    if (Multipart->Process (buffer, size) != size)
    {
        lwp_trace ("Error w/ multipart form data");

        delete Multipart;
        Multipart = 0;

        return -1;
    }

    if ( (!Multipart) || Multipart->Done)
        SignalEOF = true;

    return 0;
}

int HTTPClient::onMessageComplete ()
{
    if (!Multipart)
        Request.RunStandardHandler ();
  
    ParsingHeaders = true;

    /* Since we're now transitioning back to parsing headers, HTTPClient::Put
     * needs to start reading lines again (if it's currently reading the
     * body).  Pausing the parser returns control to HTTPClient::Put.
     */

    http_parser_pause (&Parser, 1);

    return 0;
}

void HTTPClient::Respond (Webserver::Request::Internal &) /* request parameter ignored - HTTP only ever has one request object per client */
{
    LastActivityTime = time (0);

    HeapBuffer &buffer = Request.Buffer;

    buffer.Reset ();

    buffer << "HTTP/" << Request.Version_Major << "." <<
                Request.Version_Minor << " " << Request.Status;
    
    for(List <WebserverHeader>::Element * E
            = Request.OutHeaders.First; E; E = E->Next)
    {
        buffer << "\r\n" << (** E).Name << ": " << (** E).Value;
    }

    for (Webserver::Request::Internal::Cookie * cookie = Request.Cookies;
            cookie; cookie = (Webserver::Request::Internal::Cookie *) cookie->hh.next)
    {
        if (!cookie->Changed)
            continue;

        buffer << "\r\nset-cookie: " << cookie->Name << "=" << cookie->Value;

        if (*cookie->Attr)
            buffer << "; " << cookie->Attr;
    }

    buffer << "\r\ncontent-length: " << Request.Queued () << "\r\n\r\n";

    Socket.Cork ();

    Request.EndQueue
        (1, (const char **) &buffer.Buffer, &buffer.Size);

    Request.BeginQueue ();

    buffer.Reset ();
    Socket.Uncork ();

    if (!http_should_keep_alive (&Parser))
        Socket.Close ();

    Request.Responded = true;

    /* If any data was queued while waiting to respond to this request, we'll
     * be able to process it now.
     */

    Retry (Retry_Now);
}

void HTTPClient::Tick ()
{
    if (Request.Responded && (time(0) - LastActivityTime) > Timeout)
    {
        lwp_trace ("Dropping HTTP connection due to inactivity (%s/%p)",
                Socket.GetAddress().ToString(), &Socket.GetAddress());

        Socket.Close ();
    }
}

HTTPRequest::HTTPRequest
    (Webserver::Internal &server, WebserverClient &client)
        : Webserver::Request::Internal (server, client)
{
}

size_t HTTPRequest::Put (const char * buffer, size_t size)
{
    assert (false);

    return size;
}

bool HTTPRequest::IsTransparent ()
{
    return true;
}

