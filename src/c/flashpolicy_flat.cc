
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2012 James McLaughlin.  All rights reserved.
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

lw_flashpolicy* lw_flashpolicy_new (lw_eventpump * pump)
    { return (lw_flashpolicy *) new FlashPolicy (*(Pump *) pump);
    }
void lw_flashpolicy_delete (lw_flashpolicy * fp)
    { delete (FlashPolicy *) fp;
    }
void lw_flashpolicy_host (lw_flashpolicy * fp, const char * filename)
    { ((FlashPolicy *) fp)->Host (filename);
    }
void lw_flashpolicy_host_filter (lw_flashpolicy * fp, const char * filename, lw_filter * filter)
    { ((FlashPolicy *) fp)->Host (filename, *(Filter *) filter);
    }
void lw_flashpolicy_unhost (lw_flashpolicy * fp)
    { ((FlashPolicy *) fp)->Unhost ();
    }
lw_bool lw_flashpolicy_hosting (lw_flashpolicy * fp)
    { return ((FlashPolicy *) fp)->Hosting ();
    }

AutoHandlerFlat (FlashPolicy, lw_flashpolicy, Error, error)

