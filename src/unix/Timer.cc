
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

struct TimerInternal;
void TimerTick(TimerInternal &Internal);

struct TimerInternal
{
    Lacewing::Timer   &Timer;
    PumpInternal  &EventPump;

    Lacewing::Timer::HandlerTick HandlerTick;

    #ifdef LacewingUseTimerFD
        int FD;
    #else
        #ifndef LacewingUseKQueue
            ThreadTracker Threads;
            Lacewing::Event StopEvent;
            int Interval;
        #endif
    #endif

    TimerInternal(Lacewing::Timer &_Timer, PumpInternal &_EventPump)
                    : Timer(_Timer), EventPump(_EventPump)
    {
        HandlerTick = 0;
        
        #ifdef LacewingUseTimerFD
            FD = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
            EventPump.AddRead(FD, this, (void *) TimerTick);
        #endif
    }

    ~TimerInternal()
    {
        #ifdef LacewingUseTimerFD
            close(FD);
        #endif
    }
};

void TimerTick(TimerInternal &Internal)
{
    if(Internal.HandlerTick)
        Internal.HandlerTick(Internal.Timer);
}

#ifndef LacewingUseKQueue
#ifndef LacewingUseTimerFD

    LacewingThread(LegacyTimer, TimerInternal, Internal)
    {
        for(;;)
        {
            Internal.StopEvent.Wait(Internal.Interval);

            if(Internal.StopEvent.Signalled())
                break;

            Internal.EventPump.EventPump.Post((void *) TimerTick, &Internal);
        }
    }

#endif
#endif

Lacewing::Timer::Timer(Lacewing::Pump &Pump)
{
    InternalTag = new TimerInternal(*this, *(PumpInternal *) Pump.InternalTag);
    Tag         = 0;
}

Lacewing::Timer::~Timer()
{
    delete ((TimerInternal *) InternalTag);
}

void Lacewing::Timer::Start(int Interval)
{
    Stop();

    TimerInternal &Internal = *((TimerInternal *) InternalTag);

    #ifdef LacewingUseKQueue
    
        struct kevent Change;
        EV_SET(&Change, 0, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR, 0, Interval, this);
        
        if(kevent(Internal.EventPump.Queue, &Change, 1, 0, 0, 0) == -1)
        {
            DebugOut("Timer: Failed to add timer to kqueue: " << strerror(errno));
            return;
        }
        
    #else
        #ifdef LacewingUseTimerFD
        
            itimerspec spec;

            spec.it_interval.tv_sec  = Interval / 1000;
            spec.it_interval.tv_nsec = (Interval % 1000) * 1000000;

            memcpy(&spec.it_interval, &spec.it_value, sizeof(spec.it_interval));

            timerfd_settime(Internal.FD, 0, &spec, 0);
            
        #else

            Internal.Interval = Interval;
            Internal.Threads.Start ((void *) LegacyTimer, &Internal);

        #endif
    #endif
}

void Lacewing::Timer::Stop()
{
    /* TODO: What if a tick has been posted using internal and this gets destructed? */

    TimerInternal &Internal = *((TimerInternal *) InternalTag);

    #ifdef LacewingUseKQueue

        /* TODO */

    #else
        #ifdef LacewingUseTimerFD
            itimerspec spec;
            memset(&spec, 0, sizeof(itimerspec));

            timerfd_settime(Internal.FD, 0, &spec, 0);
        #else        
            Internal.StopEvent.Signal();
            Internal.Threads.WaitUntilDead();
            Internal.StopEvent.Unsignal();
        #endif
    #endif
}

void Lacewing::Timer::ForceTick()
{
    TimerInternal &Internal = *((TimerInternal *) InternalTag);
    
    if(Internal.HandlerTick)
        Internal.HandlerTick(*this);
}

AutoHandlerFunctions(Lacewing::Timer, TimerInternal, Tick)

