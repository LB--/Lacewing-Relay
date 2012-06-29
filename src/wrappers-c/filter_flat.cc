
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

#include "../lw_common.h"

lw_filter * lw_filter_new ()
    { return (lw_filter *) new Filter;
    }
void lw_filter_delete (lw_filter * filter)
    { delete (Filter *) filter;
    }
lw_filter * lw_filter_copy (lw_filter * filter)
    { return (lw_filter *) new Filter (*(Filter *) filter);
    }
void lw_filter_set_local (lw_filter * filter, lw_addr * addr)
    { ((Filter *) filter)->Local ((Address *) addr);
    }
lw_addr * lw_filter_get_local (lw_filter * filter)
    { return (lw_addr *) ((Filter *) filter)->Local ();
    }
void lw_filter_set_remote (lw_filter * filter, lw_addr * addr)
    { ((Filter *) filter)->Remote ((Address *) addr);
    }
lw_addr * lw_filter_get_remote (lw_filter * filter)
    { return (lw_addr *) ((Filter *) filter)->Remote ();
    }
void lw_filter_set_local_port (lw_filter * filter, long port)
    { ((Filter *) filter)->LocalPort (port);
    }
long lw_filter_get_local_port (lw_filter * filter)
    { return ((Filter *) filter)->LocalPort ();
    }
void lw_filter_set_remote_port (lw_filter * filter, long port)
    { ((Filter *) filter)->RemotePort (port);
    }
long lw_filter_get_remote_port (lw_filter * filter)
    { return ((Filter *) filter)->RemotePort ();
    }
void lw_filter_set_reuse (lw_filter * filter, lw_bool reuse)
    { ((Filter *) filter)->Reuse (reuse != 0);
    }
lw_bool lw_filter_is_reuse_set (lw_filter * filter)
    { return ((Filter *) filter)->Reuse ();
    }
void lw_filter_set_ipv6 (lw_filter * filter, lw_bool enabled)
    { ((Filter *) filter)->IPv6 (enabled);
    }
lw_bool lw_filter_is_ipv6 (lw_filter * filter)
    { return ((Filter *) filter)->IPv6 ();
    }


