
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

struct EventInternal
{
    
    volatile long Signalled;
    
    #ifdef LacewingUseMPSemaphore

        MPSemaphoreID Semaphore;
        volatile long WaiterCount;
        
        EventInternal()
        {
            Signalled = 0;
            WaiterCount = 0;
            
            MPCreateSemaphore(1, 0, &Semaphore);
        }
    
        ~EventInternal()
        {
            MPDeleteSemaphore(Semaphore);
            Signalled = 1;
        }
        
    #else
        
        sem_t Semaphore;
        
        EventInternal()
        {
            Signalled = 0;
            sem_init(&Semaphore, 0, 0);
        }
        
        ~EventInternal()
        {
            sem_destroy(&Semaphore);
            Signalled = 1;
        }

    #endif

};

Lacewing::Event::Event()
{
    LacewingInitialise();

    Tag = 0;
    InternalTag = new EventInternal;
}

Lacewing::Event::~Event()
{
    delete ((EventInternal *) InternalTag);
}

bool Lacewing::Event::Signalled()
{
    return ((EventInternal *) InternalTag)->Signalled != 0;
}

void Lacewing::Event::Signal()
{
    LacewingSyncExchange(&((EventInternal *) InternalTag)->Signalled, 1);
    
    #ifdef LacewingUseMPSemaphore
    
        long CurrentWaiterCount = ((EventInternal *) InternalTag)->WaiterCount;
        
        for(int i = 0; i < CurrentWaiterCount; ++ i)
            MPSignalSemaphore(((EventInternal *) InternalTag)->Semaphore);
            
    #else
        sem_post(&((EventInternal *) InternalTag)->Semaphore);
    #endif
}

void Lacewing::Event::Unsignal()
{
    LacewingSyncExchange(&((EventInternal *) InternalTag)->Signalled, 0);
}

void Lacewing::Event::Wait(int Timeout)
{
    if(Signalled())
        return;
        
    #ifdef LacewingUseMPSemaphore
    
        LacewingSyncIncrement(&((EventInternal *) InternalTag)->WaiterCount);
    
        if(Timeout == -1)
        {
            while(((EventInternal *) InternalTag)->Signalled == 0)
                MPWaitOnSemaphore(((EventInternal *) InternalTag)->Semaphore, kDurationForever);
        }
        else
            MPWaitOnSemaphore(((EventInternal *) InternalTag)->Semaphore, kDurationMillisecond * Timeout);
        
        LacewingSyncDecrement(&((EventInternal *) InternalTag)->WaiterCount);
        
        printf("Event: Wake up\n");
        
    #else

        if(Timeout == -1)
        {
            while(((EventInternal *) InternalTag)->Signalled == 0)
                sem_wait(&((EventInternal *) InternalTag)->Semaphore);
        
            return;
        }

        timespec Time;

        #ifdef HAVE_CLOCK_GETTIME
            clock_gettime(CLOCK_REALTIME, &Time);
        #else

        #endif

        Time.tv_sec += Timeout / 1000;
        Time.tv_nsec += (Timeout % 1000) * 1000000;

        sem_timedwait(&((EventInternal *) InternalTag)->Semaphore, &Time);
    
    #endif
}

