
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

struct Timer::Internal
{
    Lacewing::Timer &Timer;
    Lacewing::Pump &Pump;

    struct
    {
        HandlerTick Tick;
    
    } Handlers;

    bool Started;

    #ifdef LacewingUseTimerFD
        int FD;
    #endif

    Event StopEvent;
    int Interval;

    Thread LegacyTimerThread;
    
    Internal (Lacewing::Timer &_Timer, Lacewing::Pump &_Pump) : Timer (_Timer), Pump (_Pump),
                      LegacyTimerThread ("LegacyTimer", (void *) LegacyTimer)
    {
        Started = false;
     
        memset (&Handlers, 0, sizeof (Handlers));

        #ifdef LacewingUseTimerFD
            FD = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK);
            EventPump.Add (FD, this, (Pump::Callback) Internal::TimerTick);
        #endif
    }

    ~ Internal ()
    {
        #ifdef LacewingUseTimerFD
            close(FD);
        #endif
    }

    static void TimerTick (Timer::Internal * internal)
    {
        if (internal->Handlers.Tick)
            internal->Handlers.Tick (internal->Timer);
    }

    static void LegacyTimer (Timer::Internal * internal)
    {
        for (;;)
        {
            internal->StopEvent.Wait (internal->Interval);

            if (internal->StopEvent.Signalled ())
                break;
            
            internal->Pump.Post ((void *) TimerTick, internal);
        }
    }
};

Timer::Timer (Lacewing::Pump &Pump)
{
    internal = new Internal (*this, Pump);
    Tag = 0;    
}

Timer::~Timer ()
{
    Stop ();

    delete internal;
}

void Timer::Start (int Interval)
{
    Stop ();

    internal->Started = true;
    internal->Pump.AddUser ();

    #ifdef LacewingUseKQueue
    
        if (internal->Pump.IsEventPump ())
        {
            EventPump::Internal * EventPump
                = ((Lacewing::EventPump *) &internal->Pump)->internal;

            struct kevent Change;
            EV_SET(&Change, 0, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR, 0, Interval, this);
            
            if (kevent (EventPump->Queue, &Change, 1, 0, 0, 0) == -1)
            {
                DebugOut("Timer: Failed to add timer to kqueue: %s", strerror(errno));
                return;
            }
        }
        else
        {
            internal->Interval = Interval;
            internal->LegacyTimerThread.Start (internal);
        }

    #else
        #ifdef LacewingUseTimerFD
        
            itimerspec spec;

            spec.it_interval.tv_sec  = Interval / 1000;
            spec.it_interval.tv_nsec = (Interval % 1000) * 1000000;

            memcpy(&spec.it_value, &spec.it_interval, sizeof (spec.it_interval));
            
            timerfd_settime (internal->FD, 0, &spec, 0);
            
        #else

            internal->Interval = Interval;
            internal->LegacyTimerThread.Start (internal);

        #endif
    #endif
}

void Timer::Stop ()
{
    /* TODO: What if a tick has been posted using internal and this gets destructed? */

    #ifndef LacewingUseTimerFD

        internal->StopEvent.Signal ();
        internal->LegacyTimerThread.Join ();
        internal->StopEvent.Unsignal ();

    #endif

    #ifdef LacewingUseKQueue

        /* TODO */

    #else
        #ifdef LacewingUseTimerFD
            itimerspec spec;
            memset (&spec, 0, sizeof(itimerspec));
            timerfd_settime (internal->FD, 0, &spec, 0);
        #endif
    #endif

    internal->Started = false;
    internal->Pump.RemoveUser ();
}

void Timer::ForceTick ()
{
    if (internal->Handlers.Tick)
        internal->Handlers.Tick (*this);
}

bool Timer::Started ()
{
    return internal->Started;
}

AutoHandlerFunctions (Timer, Tick)

