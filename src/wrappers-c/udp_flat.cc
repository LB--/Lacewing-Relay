
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

#include "../lw_common.h"

lw_udp* lw_udp_new (lw_pump * pump)
    { return (lw_udp *) new UDP (*(Pump *) pump);
    }
void lw_udp_delete (lw_udp * udp)
    { delete (UDP *) udp;
    }
void lw_udp_host (lw_udp * udp, long port)
    { ((UDP *) udp)->Host (port);
    }
void lw_udp_host_filter (lw_udp * udp, lw_filter * filter)
    { ((UDP *) udp)->Host (*(Filter *) filter);
    }
void lw_udp_host_addr (lw_udp * udp, lw_addr * addr)
    { ((UDP *) udp)->Host (*(Address *) addr);
    }
lw_bool lw_udp_hosting (lw_udp * udp)
    { return ((UDP *) udp)->Hosting ();
    }
void lw_udp_unhost (lw_udp * udp)
    { ((UDP *) udp)->Unhost ();
    }
long lw_udp_port (lw_udp * udp)
    { return ((UDP *) udp)->Port ();
    }
void lw_udp_send (lw_udp * udp, lw_addr * addr, const char * data, long size)
    { ((UDP *) udp)->Write (*(Address *) addr, data, size);
    }

AutoHandlerFlat (UDP, lw_udp, lw_udp, Receive, receive)
AutoHandlerFlat (UDP, lw_udp, lw_udp, Error, error)

