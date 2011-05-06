
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

lw_client * lw_client_new (lw_eventpump * eventpump)
    { return (lw_client *) new Lacewing::Client(*(Lacewing::EventPump *) eventpump);
    }
void lw_client_delete (lw_client * client)
    { delete (Lacewing::Client *) client;
    }
void lw_client_connect (lw_client * client, const char * host, long port)
    { ((Lacewing::Client *) client)->Connect(host, port);
    }
void lw_client_connect_addr (lw_client * client, lw_addr * address)
    { ((Lacewing::Client *) client)->Connect(*(Lacewing::Address *) address);
    }
void lw_client_disconnect (lw_client * client)
    { ((Lacewing::Client *) client)->Disconnect();
    }
lw_bool lw_client_connected (lw_client * client)
    { return ((Lacewing::Client *) client)->Connected();
    }
lw_bool lw_client_connecting (lw_client * client)
    { return ((Lacewing::Client *) client)->Connecting();
    }
lw_addr * lw_client_server_addr (lw_client * client)
    { return (lw_addr *) &((Lacewing::Client *) client)->ServerAddress();
    }
void lw_client_send (lw_client * client, const char * data, long size)
    { ((Lacewing::Client *) client)->Send(data, size);
    }
void lw_client_send_text (lw_client * client, const char * text)
    { ((Lacewing::Client *) client)->Send(text);
    }
void lw_client_disable_nagling (lw_client * client)
    { ((Lacewing::Client *) client)->DisableNagling();
    }
void lw_client_start_buffering (lw_client * client)
    { ((Lacewing::Client *) client)->StartBuffering();
    }
void lw_client_flush (lw_client * client)
    { ((Lacewing::Client *) client)->Flush();
    }

AutoHandlerFlat(Lacewing::Client, lw_client, Connect, connect)
AutoHandlerFlat(Lacewing::Client, lw_client, Disconnect, disconnect)
AutoHandlerFlat(Lacewing::Client, lw_client, Receive, receive)
AutoHandlerFlat(Lacewing::Client, lw_client, Error, error)





