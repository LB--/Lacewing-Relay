
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

#include "../../../deps/spdy/include/spdy.h"

struct SPDYRequest : public Webserver::Request::Internal
{
    SPDYRequest (Webserver::Internal &, WebserverClient &, spdy_stream *);

    spdy_stream * stream;

    size_t Put (const char * buffer, size_t size);
};

class SPDYClient : public WebserverClient
{
protected:

    spdy_ctx * spdy;

public:

    List <Webserver::Request::Internal *> Requests;

    SPDYClient (Webserver::Internal &, Lacewing::Server::Client &,
                bool secure, int version);

    ~ SPDYClient ();

    size_t Put (const char * buffer, size_t size);

    void Respond (Webserver::Request::Internal &);

    void Tick ();

    void onRequestComplete (spdy_stream * stream);

    /* libspdy callbacks */

    void onStreamCreate  (spdy_stream *, size_t num_headers, spdy_nv_pair *);
    void onStreamHeaders (spdy_stream *, size_t num_headers, spdy_nv_pair *);
    void onStreamData    (spdy_stream *, const char * data, size_t size);
    void onStreamClose   (spdy_stream *, int status_code);
};

