
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
#include "EventPump.h"

struct TimerInternal;

struct TimerInternal
{
    ThreadTracker Threads;

    EventPumpInternal &EventPump;
    Lacewing::Timer   &Timer;
 
    HANDLE TimerHandle;
    HANDLE ShutdownEvent;

    Lacewing::Timer::HandlerTick HandlerTick;

    TimerInternal(Lacewing::Timer &_Timer, EventPumpInternal &_EventPump)
                    : Timer(_Timer), EventPump(_EventPump)
    {
        ShutdownEvent = CreateEvent         (0, TRUE, FALSE, 0);
        TimerHandle   = CreateWaitableTimer (0, FALSE, 0);

        HandlerTick = 0;
    }
};

void TimerCompletion(TimerInternal &Internal)
{
    if(Internal.HandlerTick)
        Internal.HandlerTick(Internal.Timer);
}

LacewingThread(TimerThread, TimerInternal, Internal)
{
    HANDLE Events[2] = { Internal.TimerHandle, Internal.ShutdownEvent };

    for(;;)
    {
        int Result = WaitForMultipleObjects(2, Events, FALSE, INFINITE);

        if(Result != WAIT_OBJECT_0)
            break;

        Internal.EventPump.Pump.Post((void *) TimerCompletion, &Internal);
    }
}

Lacewing::Timer::Timer(Lacewing::EventPump &EventPump)
{
    InternalTag = new TimerInternal(*this, *(EventPumpInternal *) EventPump.InternalTag);
    Tag         = 0;

    ((TimerInternal *) InternalTag)->Threads.Start(TimerThread, InternalTag);
}

Lacewing::Timer::~Timer()
{
    TimerInternal &Internal = *((TimerInternal *) InternalTag);
     
    SetEvent(Internal.ShutdownEvent);
    Internal.Threads.WaitUntilDead();

    delete &Internal;
}

void Lacewing::Timer::Start(int Interval)
{
    Stop();

    TimerInternal &Internal = *((TimerInternal *) InternalTag);

    LARGE_INTEGER DueTime;
    DueTime.QuadPart = 0 - (Interval * 1000 * 10);

    if(!SetWaitableTimer(Internal.TimerHandle, &DueTime, Interval, 0, 0, 0))
    {
        LacewingAssert(false);
    }
}

void Lacewing::Timer::Stop()
{
    TimerInternal &Internal = *((TimerInternal *) InternalTag);

    CancelWaitableTimer(Internal.TimerHandle);
}

void Lacewing::Timer::ForceTick()
{
    TimerInternal &Internal = *((TimerInternal *) InternalTag);

    if(Internal.HandlerTick)
        Internal.HandlerTick(*this);
}

AutoHandlerFunctions(Lacewing::Timer, TimerInternal, Tick)

