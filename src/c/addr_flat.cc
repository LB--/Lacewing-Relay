
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011 James McLaughlin.  All rights reserved.
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

lw_addr * lw_addr_new (const char * hostname, const char * service)
    { return (lw_addr *) new Address (hostname, service);
    }
lw_addr * lw_addr_new_port (const char * hostname, long port)
    { return (lw_addr *) new Address (hostname, port);
    }
lw_addr * lw_addr_new_ex (const char * hostname, const char * service, long hints)
    { return (lw_addr *) new Address (hostname, service, hints);
    }
lw_addr * lw_addr_new_port_ex (const char * hostname, long port, long hints)
    { return (lw_addr *) new Address (hostname, port, hints);
    }
lw_addr * lw_addr_copy (lw_addr * address)
    { return (lw_addr *) new Address (*(Address *) address);
    }
void lw_addr_delete (lw_addr * address)
    { delete (Address *) address;
    }
long lw_addr_port (lw_addr * address)
    { return ((Address *) address)->Port ();
    }
void lw_addr_set_port (lw_addr * address, long port)
    { ((Address *) address)->Port (port);
    }
lw_bool lw_addr_is_ready (lw_addr * address)
    { return ((Address *) address)->Ready ();
    }
lw_bool lw_addr_is_ipv6 (lw_addr * address)
    { return ((Address *) address)->IPv6 ();
    }
lw_bool lw_addr_is_equal (lw_addr * address_a, lw_addr * address_b)
    { return (*(Address *) address_a) == (*(Address *) address_b);
    }
const char * lw_addr_to_string (lw_addr * address)
    { return ((Address *) address)->ToString ();
    }


