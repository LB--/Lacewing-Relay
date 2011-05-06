
/*
    Copyright (C) 2011 James McLaughlin

    This file is part of Lacewing.

    Lacewing is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lacewing is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Lacewing.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "../Common.h"

lw_server * lw_server_new (lw_eventpump * eventpump)
    { return (lw_server *) new Lacewing::Server(*(Lacewing::EventPump *) eventpump);
    }
void lw_server_delete (lw_server * server)
    { delete (Lacewing::Server *) server;
    }
void lw_server_host (lw_server * server, long port)
    { ((Lacewing::Server *) server)->Host(port);
    }
void lw_server_host_ex (lw_server * server, long port, lw_bool client_speaks_first)
    { ((Lacewing::Server *) server)->Host(port, client_speaks_first != 0);
    }
void lw_server_host_filter (lw_server * server, lw_filter * filter)
    { ((Lacewing::Server *) server)->Host(*(Lacewing::Filter *) filter);
    }
void lw_server_host_filter_ex (lw_server * server, lw_filter * filter, lw_bool client_speaks_first)
    { ((Lacewing::Server *) server)->Host(*(Lacewing::Filter *) filter, client_speaks_first != 0);
    }
void lw_server_unhost (lw_server * server)
    { ((Lacewing::Server *) server)->Unhost();
    }
lw_bool lw_server_hosting (lw_server * server)
    { return ((Lacewing::Server *) server)->Hosting() ? 1 : 0;
    }
long lw_server_port (lw_server * server)
    { return ((Lacewing::Server *) server)->Port();
    }
lw_bool lw_server_load_cert_file (lw_server * server, const char * filename, const char * passphrase)
    { return ((Lacewing::Server *) server)->LoadCertificateFile(filename, passphrase);
    }
lw_bool lw_server_load_sys_cert (lw_server * server, const char * store_name, const char * common_name, const char * location)
    { return ((Lacewing::Server *) server)->LoadSystemCertificate(store_name, common_name, location);
    }
lw_bool lw_server_cert_loaded (lw_server * server)
    { return ((Lacewing::Server *) server)->CertificateLoaded();
    }
lw_i64 lw_server_bytes_sent (lw_server * server)
    { return ((Lacewing::Server *) server)->BytesSent();
    }
lw_i64 lw_server_bytes_received (lw_server * server)
    { return ((Lacewing::Server *) server)->BytesReceived();
    }
void lw_server_disable_nagling (lw_server * server)
    { ((Lacewing::Server *) server)->DisableNagling();
    }
lw_addr * lw_server_client_address (lw_server_client * client)
    { return (lw_addr *) &((Lacewing::Server::Client *) client)->GetAddress();
    }
void lw_server_client_send (lw_server_client * client, const char * data, long size)
    { ((Lacewing::Server::Client *) client)->Send(data, size);
    }
void lw_server_client_send_writable (lw_server_client * client, char * data, long size)
    { ((Lacewing::Server::Client *) client)->SendWritable(data, size);
    }
void lw_server_client_start_buffering (lw_server_client * client)
    { ((Lacewing::Server::Client *) client)->StartBuffering();
    }
void lw_server_client_flush (lw_server_client * client)
    { ((Lacewing::Server::Client *) client)->Flush();
    }
void lw_server_client_disconnect (lw_server_client * client)
    { ((Lacewing::Server::Client *) client)->Disconnect();
    }

AutoHandlerFlat(Lacewing::Server, lw_server, Connect, connect)
AutoHandlerFlat(Lacewing::Server, lw_server, Disconnect, disconnect)
AutoHandlerFlat(Lacewing::Server, lw_server, Receive, receive)
AutoHandlerFlat(Lacewing::Server, lw_server, Error, error)




