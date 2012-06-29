
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2012 James McLaughlin.  All rights reserved.
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

static void spdy_emit (spdy_ctx * ctx, const char * buffer, size_t size)
{
    lwp_trace ("SPDY emit " lwp_fmt_size " bytes", size);
    lw_dump (buffer, size);

    ((SPDYClient *) spdy_ctx_get_tag (ctx))->Socket.Write (buffer, size);
}

static void spdy_stream_create
    (spdy_ctx * ctx, spdy_stream * stream, size_t num, spdy_nv_pair * headers)
{
    ((SPDYClient *) spdy_ctx_get_tag (ctx))
        ->onStreamCreate (stream, num, headers);
}

static void spdy_stream_headers
    (spdy_ctx * ctx, spdy_stream * stream, size_t num, spdy_nv_pair * headers)
{
    ((SPDYClient *) spdy_ctx_get_tag (ctx))
        ->onStreamHeaders (stream, num, headers);
}

static void spdy_stream_data
    (spdy_ctx * ctx, spdy_stream * stream, const char * data, size_t size)
{
    ((SPDYClient *) spdy_ctx_get_tag (ctx))
        ->onStreamData (stream, data, size);
}

static void spdy_stream_close
    (spdy_ctx * ctx, spdy_stream * stream, int status_code)
{
    ((SPDYClient *) spdy_ctx_get_tag (ctx))
        ->onStreamClose (stream, status_code);
}

static spdy_config config = { 0 };

SPDYClient::SPDYClient
    (Webserver::Internal &server, Server::Client &socket,
      bool secure, int version) : WebserverClient (server, socket, secure)
{
    if (!config.emit)
    {
        config.is_server = true;

        config.emit = ::spdy_emit;

        config.on_stream_create = ::spdy_stream_create;
        config.on_stream_headers = ::spdy_stream_headers;
        config.on_stream_data = ::spdy_stream_data;
        config.on_stream_close = ::spdy_stream_close;
    }

    spdy = spdy_ctx_new (&config, version, 0, 0);
    spdy_ctx_set_tag (spdy, this);

    lwp_trace ("SPDY context created @ %p", spdy);

    /* When the retry mode is Retry_MoreData and Put can't consume everything,
     * Put will be called again as soon as more data arrives.
     */

    Retry (Retry_MoreData);
}

SPDYClient::~SPDYClient ()
{
    for (List <Webserver::Request::Internal *>::Element * E
            = Requests.First; E; E = E->Next)
    {
        Webserver::Request::Internal * request = (** E);

        if (Server.Handlers.Disconnect)
            Server.Handlers.Disconnect (Server.Webserver, *request);
    }

    spdy_ctx_delete (spdy);
}

size_t SPDYClient::Put (const char * buffer, size_t size)
{
    lwp_trace (lwp_fmt_size " bytes -> SPDY", size);
    lw_dump (buffer, size);

    int res = spdy_data (spdy, buffer, &size);

    lwp_trace ("SPDY processed " lwp_fmt_size " bytes", size);

    if (res != SPDY_E_OK)
    {
        lwp_trace ("SPDY error: %s", spdy_error_string (res));

        Socket.Close ();

        return size;
    }

    return size;
}

void SPDYClient::Respond (Webserver::Request::Internal &request)
{
    spdy_stream * stream = (spdy_stream *) request.Tag;
    
    assert (!spdy_stream_open_remote (stream));

    Socket.Cork ();

    spdy_nv_pair * headers = (spdy_nv_pair *)
        alloca (sizeof (spdy_nv_pair) * (request.OutHeaders.Size + 3));

    int n = 0;

    /* version header */

    char version [16];

    sprintf (version, "HTTP/%d.%d",
                (int) request.Version_Major, (int) request.Version_Minor);

    {   spdy_nv_pair &pair = headers [n ++];

        pair.name_len = strlen (pair.name = (char *)
             (spdy_active_version (spdy) == 2 ? "version" : ":version"));

        pair.value_len = strlen (pair.value = version);
    }

    /* status header */

    {   spdy_nv_pair &pair = headers [n ++];

        pair.name_len = strlen (pair.name = (char *)
             (spdy_active_version (spdy) == 2 ? "status" : ":status"));

        pair.value_len = strlen (pair.value = request.Status);
    }

    /* content-length header */

    size_t length = request.Queued ();

    char length_str [24];
    sprintf (length_str, lwp_fmt_size, length);

    {   spdy_nv_pair &pair = headers [n ++];

        pair.name_len = strlen (pair.name = (char *) "content-length");
        pair.value_len = strlen (pair.value = length_str);
    }

    for(List <WebserverHeader>::Element * E
            = request.OutHeaders.First; E; E = E->Next)
    {
        WebserverHeader &header = (** E);
        spdy_nv_pair &pair = headers [n ++];

        pair.name = header.Name;
        pair.name_len = strlen (header.Name);

        pair.value = header.Value;
        pair.value_len = strlen (header.Value);
    }

    if (length > 0)
    {
        int res = spdy_stream_write_headers (stream, n, headers, 0);

        if (res != SPDY_E_OK)
        {
            lwp_trace ("Error writing headers: %s", spdy_error_string (res));
        }

    }
    else
    {
        /* Writing headers with SPDY_FLAG_FIN when the stream is already closed
         * to the remote will cause the stream to be deleted.
         */

        spdy_stream_write_headers (stream, n, headers, SPDY_FLAG_FIN);
        stream = 0;
    }

    lwp_trace ("SPDY: %d headers -> stream @ %p, content length " lwp_fmt_size,
                    n, stream, length);

    Socket.Write (request, length);

    request.EndQueue ();
    request.BeginQueue ();

    if (length > 0)
    {
        spdy_stream_write_data (stream, 0, 0, SPDY_FLAG_FIN);
        stream = 0;
    }

    Socket.Uncork ();

    delete &request;
}

void SPDYClient::Tick ()
{

}

void SPDYClient::onStreamCreate
     (spdy_stream * stream, size_t num_headers, spdy_nv_pair * headers)
{
    SPDYRequest * request = new (std::nothrow)
        SPDYRequest (Server, *this, stream);

    if (!request)
    {
        spdy_stream_close (stream, SPDY_STATUS_INTERNAL_ERROR);
        return;
    }

    request->Tag = stream;
    spdy_stream_set_tag (stream, request);

    onStreamHeaders (stream, num_headers, headers);

    if (!spdy_stream_open_remote (stream))
    {
        /* Stream is already half-closed to the remote endpoint, so we
         * aren't expecting a request body.
         */

        onRequestComplete (stream);
    }
}

void SPDYClient::onRequestComplete (spdy_stream * stream)
{
    SPDYRequest * request = (SPDYRequest *) spdy_stream_get_tag (stream);

    lwp_trace ("SPDY request complete: %p", request);

    request->RunStandardHandler ();
}

void SPDYClient::onStreamHeaders
     (spdy_stream * stream, size_t num_headers, spdy_nv_pair * headers)
{
    SPDYRequest * request = (SPDYRequest *) spdy_stream_get_tag (stream);

    lwp_trace ("SPDY: Got %d headers for stream @ %p", num_headers, stream);

    for (size_t i = 0; i < num_headers; ++ i)
    { 
        spdy_nv_pair * header = &headers [i];

        if (spdy_active_version (spdy) == 2)
        {
            if (!strcmp (header->name, "version"))
            {
                request->In_Version (header->value_len, header->value);
                continue;
            }

            if (!strcmp (header->name, "method"))
            {
                request->In_Method (header->value_len, header->value);
                continue;
            }

            if (!strcmp (header->name, "url"))
            {
                request->In_URL (header->value_len, header->value);
                continue;
            }
        }
        else
        {
            if (!strcmp (header->name, ":version"))
            {
                request->In_Version (header->value_len, header->value);
                continue;
            }

            if (!strcmp (header->name, ":method"))
            {
                request->In_Method (header->value_len, header->value);
                continue;
            }

            if (!strcmp (header->name, ":scheme"))
               continue;

            if (!strcmp (header->name, ":path"))
            {
               request->In_URL (header->value_len, header->value);
               continue;
            }
        }

        /* Each header may contain multiple values separated by 0 octets. */

        char * value_start = header->value;
        size_t value_len = 0;

        for (size_t i = 0 ;; ++ i)
        {
            if (i == header->value_len)
            {
                request->In_Header (header->name_len, header->name,
                                    value_len, value_start);

                break;
            }

            if (header->value [i] == '\0')
            {
                request->In_Header (header->name_len, header->name,
                                    value_len, value_start);

                value_len = 0;
                value_start = header->value + i + 1;

                continue;
            }

            ++ value_len;
        }
    }
}

void SPDYClient::onStreamData
     (spdy_stream * stream, const char * data, size_t size)
{
    SPDYRequest * request = (SPDYRequest *) spdy_stream_get_tag (stream);

    request->Buffer.Add (data, size);

    if (!spdy_stream_open_remote (stream))
        onRequestComplete (stream);
}

void SPDYClient::onStreamClose
     (spdy_stream * stream, int status_code)
{
    SPDYRequest * request = (SPDYRequest *) spdy_stream_get_tag (stream);

    if (!request)
        return;

    delete request;
}

SPDYRequest::SPDYRequest
    (Webserver::Internal &server, WebserverClient &client, spdy_stream * _stream)
        : Webserver::Request::Internal (server, client), stream (_stream)
{
    BeginQueue ();
}

size_t SPDYRequest::Put (const char * buffer, size_t size)
{
    lwp_trace ("SPDY: Writing " lwp_fmt_size " bytes of data", size);

    spdy_stream_write_data (stream, buffer, size, 0);

    return size;
}


