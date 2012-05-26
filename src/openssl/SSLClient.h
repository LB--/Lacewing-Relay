
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

namespace Lacewing
{
    class SSLClient
    {
        ssl_st * SSL;
        BIO * InternalBIO, * ExternalBIO;

        void Pump ();
        bool Pumping;

        int WriteCondition;

    public:

        SSLClient (SSL_CTX *);
        ~ SSLClient ();


        /* Data coming from the network passes through this Stream first */

        struct Downstream : public Stream
        {
            SSLClient &Client;

            Downstream (SSLClient &);

            size_t Put (const char * buffer, size_t size);

            friend class Lacewing::SSLClient;

        } Downstream;


        /* Data destined for the network passes through this Stream first */

        struct Upstream : public Stream
        {
            SSLClient &Client;

            Upstream (SSLClient &);

            size_t Put (const char * buffer, size_t size);

            friend class Lacewing::SSLClient;

        } Upstream;

    };
}



