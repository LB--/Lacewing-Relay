
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

lw_timer * lw_timer_new (lw_eventpump * eventpump)
    { return (lw_timer *) new Lacewing::Timer(*(Lacewing::EventPump *) eventpump);
    }
void lw_timer_delete (lw_timer * timer)
    { delete (Lacewing::Timer *) timer;
    }
void lw_timer_start (lw_timer * timer, long milliseconds)
    { ((Lacewing::Timer *) timer)->Start(milliseconds);
    }
void lw_timer_stop (lw_timer * timer)
    { ((Lacewing::Timer *) timer)->Stop();
    }
void lw_timer_force_tick (lw_timer * timer)
    { ((Lacewing::Timer *) timer)->ForceTick();
    }
void lw_timer_ontick (lw_timer * timer, lw_timer_handler_tick ontick)
    { ((Lacewing::Timer *) timer)->onTick((Lacewing::Timer::HandlerTick) ontick);
    }




