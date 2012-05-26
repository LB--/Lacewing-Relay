
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

SSLClient::Downstream::Downstream (SSLClient &_Client)
    : Client (_Client)
{
}

SSLClient::Upstream::Upstream (SSLClient &_Client)
    : Client (_Client)
{
}

SSLClient::SSLClient (SSL_CTX * context)
    : Downstream (*this), Upstream (*this)
{
    Pumping = false;
    WriteCondition = -1;

    SSL = SSL_new (context);

    /* TODO : I'm not really happy with the extra layer of buffering
     * that BIO pairs introduce.  Is there a better way to do this?
     */

    BIO_new_bio_pair (&InternalBIO, 0, &ExternalBIO, 0);
    SSL_set_bio (SSL, InternalBIO, InternalBIO);

    SSL_set_accept_state (SSL);
}

SSLClient::~ SSLClient ()
{
    SSL_free (SSL);
    BIO_free (ExternalBIO);
}

size_t SSLClient::Downstream::Put (const char * buffer, size_t size)
{
    int bytes = BIO_write (Client.ExternalBIO, buffer, size);

    Client.Pump ();

    if (bytes < 0)
        return 0;

    LacewingAssert (bytes == size);

    /* If OpenSSL was waiting for some more incoming data before we could
     * write something, signal Stream::WriteReady to have Put called again.
     */

    if (Client.WriteCondition == SSL_ERROR_WANT_READ)
    {
        Client.WriteCondition = -1;
        Client.Upstream.WriteReady ();
    }

    return bytes;
}

size_t SSLClient::Upstream::Put (const char * buffer, size_t size)
{
    int bytes = SSL_write (Client.SSL, buffer, size);
    int error = bytes < 0 ? SSL_get_error (Client.SSL, bytes) : -1;

    Client.Pump ();

    if (error != -1)
    {
        if (error == SSL_ERROR_WANT_READ)
            Client.WriteCondition = error;

        return 0;
    }

    return bytes;
}

void SSLClient::Pump ()
{
    if (Pumping)
        return; /* prevent stack overflow (we loop anyway) */

    Pumping = true;

    {   char buffer [DefaultBufferSize];

        for (;;)
        {
            bool exit = true;

            /* First check for any new data destined for the network */

            {   int bytes = BIO_read (ExternalBIO, buffer, sizeof (buffer));

                if (bytes > 0)
                {
                    exit = false;
                    Upstream.Data (buffer, bytes);
                }
            }


            /* Now check for any data that's been decrypted and is ready to use */

            {   int bytes = SSL_read (SSL, buffer, sizeof (buffer));

                if (bytes > 0)
                {
                    exit = false;
                    Downstream.Data (buffer, bytes);
                }
                else
                {
                    if (SSL_get_error (SSL, bytes) == SSL_ERROR_WANT_WRITE)
                        continue;
                }
            }

            if (BIO_ctrl_pending (ExternalBIO) > 0)
                continue;

            if (exit)
                break;
        }
    }

    Pumping = false;
}


