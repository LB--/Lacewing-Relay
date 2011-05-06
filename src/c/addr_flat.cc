
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

lw_addr * lw_addr_new ()
    { return (lw_addr *) new Lacewing::Address();
    }
lw_addr * lw_addr_new_ip (unsigned int ip, long port)
    { return (lw_addr *) new Lacewing::Address(ip, port);
    }
lw_addr * lw_addr_new_name (const char * hostname, long port, lw_bool blocking)
    { return (lw_addr *) new Lacewing::Address(hostname, port, blocking ? 1 : 0);
    }
lw_addr * lw_addr_copy (lw_addr * address)
    { return (lw_addr *) new Lacewing::Address(*(Lacewing::Address *) address);
    }
void lw_addr_delete (lw_addr * address)
    { delete (Lacewing::Address *) address;
    }
long lw_addr_port (lw_addr * address)
    { return ((Lacewing::Address *) address)->Port();
    }
void lw_addr_set_port (lw_addr * address, long port)
    { ((Lacewing::Address *) address)->Port(port);
    }
lw_bool lw_addr_is_ready (lw_addr * address)
    { return ((Lacewing::Address *) address)->Ready();
    }
long lw_addr_ip (lw_addr * address)
    { return ((Lacewing::Address *) address)->IP();
    }
unsigned char lw_addr_ip_byte (lw_addr * address, long index)
    { return ((Lacewing::Address *) address)->IP_Byte(index);
    }
const char * lw_addr_tostring (lw_addr * address)
    { return ((Lacewing::Address *) address)->ToString();
    }

