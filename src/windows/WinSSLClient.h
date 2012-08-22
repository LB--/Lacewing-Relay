
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
    class WinSSLClient
    {
        int Status;

        char Flags;

        const static char Flag_GotContext = 1;
        const static char Flag_GotHello = 2;
        const static char Flag_HandshakeComplete = 4;

        CredHandle ServerCreds;
        
        CtxtHandle Context;
        SecPkgContext_StreamSizes Sizes;
        
    public:

        WinSSLClient (CredHandle);
        ~ WinSSLClient ();


        /* Data coming from the network passes through this Stream first */

        class Downstream : public Stream
        {
            size_t ProcHandshakeData (const char * buffer, size_t size);
            size_t ProcMessageData (const char * buffer, size_t size);

        public:

            using Stream::Data;

            WinSSLClient &Client;

            Downstream (WinSSLClient &);

            size_t Put (const char * buffer, size_t size);

            friend class Lacewing::WinSSLClient;

        } Downstream;


        /* Data destined for the network passes through this Stream first */

        struct Upstream : public Stream
        {
            using Stream::Data;

            WinSSLClient &Client;

            Upstream (WinSSLClient &);

            size_t Put (const char * buffer, size_t size);

            friend class Lacewing::WinSSLClient;

        } Upstream;

    };
}



