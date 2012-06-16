
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

lw_stream * lw_client_new (lw_pump * pump)
    { return (lw_stream *) new Client (*(Pump *) pump);
    }
void lw_client_delete (lw_stream * client)
    { delete (Client *) client;
    }
void lw_client_connect (lw_stream * client, const char * host, long port)
    { ((Client *) client)->Connect (host, port);
    }
void lw_client_connect_addr (lw_stream * client, lw_addr * address)
    { ((Client *) client)->Connect (*(Address *) address);
    }
void lw_client_close (lw_stream * client)
    { ((Client *) client)->Close ();
    }
lw_bool lw_client_connected (lw_stream * client)
    { return ((Client *) client)->Connected ();
    }
lw_bool lw_client_connecting (lw_stream * client)
    { return ((Client *) client)->Connecting ();
    }
lw_addr * lw_client_server_addr (lw_stream * client)
    { return (lw_addr *) &((Client *) client)->ServerAddress ();
    }

AutoHandlerFlat (Client, lw_stream, lw_client, Connect, connect)
AutoHandlerFlat (Client, lw_stream, lw_client, Disconnect, disconnect)
AutoHandlerFlat (Client, lw_stream, lw_client, Receive, receive)
AutoHandlerFlat (Client, lw_stream, lw_client, Error, error)





