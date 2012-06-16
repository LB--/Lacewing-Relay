
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

lw_server * lw_server_new (lw_pump * pump)
    { return (lw_server *) new Server (*(Pump *) pump);
    }
void lw_server_delete (lw_server * server)
    { delete (Server *) server;
    }
void lw_server_host (lw_server * server, long port)
    { ((Server *) server)->Host (port);
    }
void lw_server_host_filter (lw_server * server, lw_filter * filter)
    { ((Server *) server)->Host (*(Filter *) filter);
    }
void lw_server_unhost (lw_server * server)
    { ((Server *) server)->Unhost ();
    }
lw_bool lw_server_hosting (lw_server * server)
    { return ((Server *) server)->Hosting () ? 1 : 0;
    }
long lw_server_port (lw_server * server)
    { return ((Server *) server)->Port ();
    }
lw_bool lw_server_load_cert_file (lw_server * server, const char * filename, const char * passphrase)
    { return ((Server *) server)->LoadCertificateFile (filename, passphrase);
    }
lw_bool lw_server_load_sys_cert (lw_server * server, const char * store_name, const char * common_name, const char * location)
    { return ((Server *) server)->LoadSystemCertificate (store_name, common_name, location);
    }
lw_bool lw_server_cert_loaded (lw_server * server)
    { return ((Server *) server)->CertificateLoaded ();
    }
lw_addr * lw_server_client_address (lw_stream * client)
    { return (lw_addr *) &((Server::Client *) client)->GetAddress ();
    }
void lw_server_client_write (lw_stream * client, const char * data, long size)
    { ((Server::Client *) client)->Write (data, size);
    }
void lw_server_client_write_text (lw_stream * client, const char * text)
    { ((Server::Client *) client)->Write (text);
    }
void lw_server_client_close (lw_stream * client)
    { ((Server::Client *) client)->Close ();
    }
lw_stream * lw_server_client_next (lw_stream * client)
    { return (lw_stream *) ((Server::Client *) client)->Next ();
    }

AutoHandlerFlat (Server, lw_server, lw_server, Connect, connect)
AutoHandlerFlat (Server, lw_server, lw_server, Disconnect, disconnect)
AutoHandlerFlat (Server, lw_server, lw_server, Receive, receive)
AutoHandlerFlat (Server, lw_server, lw_server, Error, error)




