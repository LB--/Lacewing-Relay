
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

lw_filter * lw_filter_new ()
    { return (lw_filter *) new Lacewing::Filter;
    }
void lw_filter_delete (lw_filter * filter)
    { delete (Lacewing::Filter *) filter;
    }
void lw_filter_set_local_ip (lw_filter * filter, long ip)
    { ((Lacewing::Filter *) filter)->LocalIP(ip);
    }
long lw_filter_get_local_ip (lw_filter * filter)
    { return ((Lacewing::Filter *) filter)->LocalIP();
    }
void lw_filter_set_remote_addr (lw_filter * filter, lw_addr * addr)
    { ((Lacewing::Filter *) filter)->RemoteAddress(*(Lacewing::Address *) addr);
    }
lw_addr * lw_filter_get_remote_addr (lw_filter * filter)
    { return (lw_addr *) &((Lacewing::Filter *) filter)->RemoteAddress();
    }
void lw_filter_set_local_port (lw_filter * filter, long port)
    { ((Lacewing::Filter *) filter)->LocalPort(port);
    }
long lw_filter_get_local_port (lw_filter * filter)
    { return ((Lacewing::Filter *) filter)->LocalPort();
    }

