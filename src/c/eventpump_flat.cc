
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

lw_eventpump * lw_eventpump_new ()
    { return (lw_eventpump *) new Lacewing::EventPump;
    }
void lw_eventpump_delete (lw_eventpump * eventpump)
    { delete (Lacewing::EventPump *) eventpump;
    }
void lw_eventpump_tick (lw_eventpump * eventpump)
    { ((Lacewing::EventPump *) eventpump)->Tick();
    }
void lw_eventpump_start_event_loop (lw_eventpump * eventpump)
    { ((Lacewing::EventPump *) eventpump)->StartEventLoop();
    }
void lw_eventpump_start_sleepy_ticking (lw_eventpump * eventpump, void (LacewingHandler * on_tick_needed) (lw_eventpump))
    { ((Lacewing::EventPump *) eventpump)->StartSleepyTicking((void (LacewingHandler *) (Lacewing::EventPump &)) on_tick_needed);
    }

