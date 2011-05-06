
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

lw_udp* lw_udp_new (lw_eventpump * eventpump)
    { return (lw_udp *) new Lacewing::UDP(*(Lacewing::EventPump *) eventpump);
    }
void lw_udp_delete (lw_udp * udp)
    { delete (Lacewing::UDP *) udp;
    }
void lw_udp_host (lw_udp * udp, long port)
    { ((Lacewing::UDP *) udp)->Host(port);
    }
void lw_udp_host_filter (lw_udp * udp, lw_filter * filter)
    { ((Lacewing::UDP *) udp)->Host(*(Lacewing::Filter *) filter);
    }
void lw_udp_host_addr (lw_udp * udp, lw_addr * addr)
    { ((Lacewing::UDP *) udp)->Host(*(Lacewing::Address *) addr);
    }
void lw_udp_unhost (lw_udp * udp)
    { ((Lacewing::UDP *) udp)->Unhost();
    }
long lw_udp_port (lw_udp * udp)
    { return ((Lacewing::UDP *) udp)->Port();
    }
lw_i64 lw_udp_bytes_sent (lw_udp * udp)
    { return ((Lacewing::UDP *) udp)->BytesSent();
    }
lw_i64 lw_udp_bytes_received (lw_udp * udp)
    { return ((Lacewing::UDP *) udp)->BytesReceived();
    }
void lw_udp_send (lw_udp * udp, lw_addr * addr, const char * data, long size)
    { ((Lacewing::UDP *) udp)->Send(*(Lacewing::Address *) addr, data, size);
    }

AutoHandlerFlat(Lacewing::UDP, lw_udp, Receive, receive)
AutoHandlerFlat(Lacewing::UDP, lw_udp, Error, error)

