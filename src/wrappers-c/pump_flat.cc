
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

#include "../lw_common.h"

void lw_pump_delete (lw_pump * pump)
    { delete (Pump *) pump;
    }
void lw_pump_add_user (lw_pump * pump)
    { ((Pump *) pump)->AddUser ();
    }
void lw_pump_remove_user (lw_pump * pump)
    { ((Pump *) pump)->RemoveUser ();
    }
lw_bool lw_pump_in_use (lw_pump * pump)
    { return ((Pump *) pump)->InUse ();
    }
void lw_pump_remove (lw_pump * pump, lw_pump_watch * watch)
    { ((Pump *) pump)->Remove ((Pump::Watch *) watch);
    }
void lw_pump_post (lw_pump * pump, void * fn, void * param)
    { ((Pump *) pump)->Post (fn, param);
    }
lw_bool lw_pump_is_eventpump (lw_pump * pump)
    { return ((Pump *) pump)->IsEventPump ();
    }

#ifdef _WIN32

  lw_pump_watch* lw_pump_add
      (lw_pump * pump, HANDLE handle, void * tag, lw_pump_callback callback)
  {
      return (lw_pump_watch *) ((Pump *) pump)->Add (handle, tag, callback);
  }

  void lw_pump_update_callbacks
        (lw_pump * pump, lw_pump_watch * watch, void * tag, lw_pump_callback callback)
  {
      ((Pump *) pump)->UpdateCallbacks ((Pump::Watch *) watch, tag, callback);
  }

#else

  lw_pump_watch* lw_pump_add
    (lw_pump * pump, int FD, void * tag, lw_pump_callback onReadReady,
        lw_pump_callback onWriteReady, lw_bool edge_triggered)
  {
      return (lw_pump_watch *) ((Pump *) pump)->Add
                (FD, tag, onReadReady, onWriteReady, edge_triggered != 0);
  }

  void lw_pump_update_callbacks
    (lw_pump * pump, lw_pump_watch * watch, void * tag, lw_pump_callback onReadReady,
          lw_pump_callback onWriteReady, lw_bool edge_triggered)
  {
      ((Pump *) pump)->UpdateCallbacks
          ((Pump::Watch *) watch, tag, onReadReady, onWriteReady, edge_triggered != 0);
  }

#endif


