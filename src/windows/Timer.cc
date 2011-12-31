
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

static void TimerThread (Timer::Internal *);

struct Timer::Internal
{
    Lacewing::Pump &Pump;
    Lacewing::Timer &Timer;
    
    Thread TimerThread;

    HANDLE TimerHandle;
    HANDLE ShutdownEvent;

    bool Started;

    struct
    {
        HandlerTick Tick;

    } Handlers;

    Internal (Lacewing::Timer &_Timer, Lacewing::Pump &_Pump)
                : Timer (_Timer), Pump (_Pump),
                    TimerThread ("Timer", (void *) ::TimerThread)
    {
        ShutdownEvent = CreateEvent         (0, TRUE, FALSE, 0);
        TimerHandle   = CreateWaitableTimer (0, FALSE, 0);

        memset (&Handlers, 0, sizeof (Handlers));

        Started = false;
    }
};

static void TimerCompletion (Timer::Internal * internal)
{
    if (internal->Handlers.Tick)
        internal->Handlers.Tick (internal->Timer);
}

void TimerThread (Timer::Internal * internal)
{
    HANDLE Events[2] = { internal->TimerHandle, internal->ShutdownEvent };

    for(;;)
    {
        int Result = WaitForMultipleObjects(2, Events, FALSE, INFINITE);

        if(Result != WAIT_OBJECT_0)
        {
            DebugOut ("Got result %d", Result);
            break;
        }

        internal->Pump.Post ((void *) TimerCompletion, internal);
    }
}

Timer::Timer (Lacewing::Pump &Pump)
{
    internal = new Internal (*this, Pump);
    Tag = 0;

    internal->TimerThread.Start (internal);
}

Timer::~Timer ()
{
    SetEvent (internal->ShutdownEvent);

    Stop ();

    internal->TimerThread.Join ();

    delete internal;
}

void Timer::Start (int Interval)
{
    Stop ();

    LARGE_INTEGER DueTime;
    DueTime.QuadPart = 0 - (Interval * 1000 * 10);

    if (!SetWaitableTimer (internal->TimerHandle, &DueTime, Interval, 0, 0, 0))
    {
        LacewingAssert (false);
    }

    internal->Started = true;
    internal->Pump.AddUser ();
}

void Timer::Stop ()
{
    if (!Started ())
        return;

    CancelWaitableTimer (internal->TimerHandle);

    internal->Started = false;
    internal->Pump.RemoveUser ();
}

bool Timer::Started ()
{
    return internal->Started;
}

void Timer::ForceTick ()
{
    if (internal->Handlers.Tick)
        internal->Handlers.Tick (*this);
}

AutoHandlerFunctions (Timer, Tick)

