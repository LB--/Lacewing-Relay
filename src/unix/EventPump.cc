
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

struct EventPumpInternal
{  
    Lacewing::EventPump &EventPump;

    int Queue;

    EventPumpInternal(Lacewing::EventPump &_EventPump, int MaxHint) : EventPump(_EventPump)
    {
        #ifdef LacewingUseEPoll
            Queue = epoll_create(MaxHint);
        #endif

        #ifdef LacewingUseKQueue
            Queue = kqueue();
        #endif
    }
};

Lacewing::EventPump::EventPump(int MaxHint)
{
    EPInternalTag = new EventPumpInternal(*this, MaxHint);
    EPTag         = 0;
}

Lacewing::EventPump::~EventPump()
{
    delete ((EventPumpInternal *) EPInternalTag);
}

const int MaxEvents = 32;

Lacewing::Error * Lacewing::EventPump::Tick()
{
    EventPumpInternal &Internal = *((EventPumpInternal *) EPInternalTag);

    #ifdef LacewingUseEPoll
    
        epoll_event EPollEvents [MaxEvents];
        int Count = epoll_wait(Internal.Queue, EPollEvents, MaxEvents, 0);
    
        for(int i = 0; i < Count; ++ i)
        {
            epoll_event &EPollEvent = EPollEvents[i];

            Ready (EPollEvent.data.ptr,
                (EPollEvent.events & EPOLLRDHUP) != 0 || (EPollEvent.events & EPOLLHUP) != 0,
                    (EPollEvent.events & EPOLLIN) != 0, (EPollEvent.events & EPOLLOUT) != 0);
        }
   
    #endif
    
    #ifdef LacewingUseKQueue
    
        struct kevent KEvents [MaxEvents];

        timespec Zero;
        memset(&Zero, 0, sizeof(Zero));
        
        int Count = kevent(Internal.Queue, 0, 0, KEvents, MaxEvents, &Zero);

        for(int i = 0; i < Count; ++ i)
        {
            struct kevent &KEvent = KEvents[i];

            Ready (KEvent.udata, (KEvent.flags & EV_EOF) != 0, KEvent.filter == EVFILT_READ,
                        KEvent.filter == EVFILT_WRITE);
        }
        
    #endif

    return 0;
}

Lacewing::Error * Lacewing::EventPump::StartEventLoop()
{
    EventPumpInternal &Internal = *((EventPumpInternal *) EPInternalTag);

    for(;;)
    {
        #ifdef LacewingUseEPoll
        
            epoll_event EPollEvents [MaxEvents];
            int Count = epoll_wait(Internal.Queue, EPollEvents, MaxEvents, -1);
        
            for(int i = 0; i < Count; ++ i)
            {
                epoll_event &EPollEvent = EPollEvents[i];

                Ready (EPollEvent.data.ptr, (EPollEvent.events & EPOLLRDHUP) != 0 ||
                        (EPollEvent.events & EPOLLHUP) != 0, (EPollEvent.events & EPOLLIN) != 0,
                        (EPollEvent.events & EPOLLOUT) != 0);
            }
       
        #endif
        
        #ifdef LacewingUseKQueue
        
            struct kevent KEvents [MaxEvents];
            int Count = kevent(Internal.Queue, 0, 0, KEvents, MaxEvents, 0);

            for(int i = 0; i < Count; ++ i)
            {
                struct kevent &KEvent = KEvents[i];
                    
                if(KEvent.filter == EVFILT_TIMER)
                {
                    ((Lacewing::Timer *) KEvent.udata)->ForceTick();
                }
                else
                {
                    Ready (KEvent.udata, (KEvent.flags & EV_EOF) != 0, KEvent.filter == EVFILT_READ,
                                            KEvent.filter == EVFILT_WRITE);
                }
            }
            
        #endif
    }
        
    return 0;
}

Lacewing::Error * Lacewing::EventPump::StartSleepyTicking(void (LacewingHandler * onTickNeeded) (Lacewing::EventPump &EventPump))
{
    EventPumpInternal &Internal = *((EventPumpInternal *) EPInternalTag);

    /* TODO */

    return 0;
}

void Lacewing::EventPump::AddRead(int FD, void * Tag)
{
    EventPumpInternal &Internal = *((EventPumpInternal *) EPInternalTag);

    #ifdef LacewingUseEPoll
    
        epoll_event Event;
        memset(&Event, 0, sizeof(epoll_event));

        Event.events = EPOLLIN | EPOLLET;
        Event.data.ptr = Tag;

        if(epoll_ctl(Internal.Queue, EPOLL_CTL_ADD, FD, &Event) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return;
        }
            
    #endif
    
    #ifdef LacewingUseKQueue
    
        struct kevent Change;
        EV_SET(&Change, FD, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, Tag); /* EV_CLEAR = edge triggered */
        
        if(kevent(Internal.Queue, &Change, 1, 0, 0, 0) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return;
        }
        
    #endif
}

void Lacewing::EventPump::AddReadWrite(int FD, void * Tag)
{
    EventPumpInternal &Internal = *((EventPumpInternal *) EPInternalTag);

    #ifdef LacewingUseEPoll
    
        epoll_event Event;
        memset(&Event, 0, sizeof(epoll_event));

        Event.events = EPOLLIN | EPOLLOUT | EPOLLET;
        Event.data.ptr = Tag;

        if(epoll_ctl(Internal.Queue, EPOLL_CTL_ADD, FD, &Event) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return;
        }
            
    #endif
    
    #ifdef LacewingUseKQueue
    
        struct kevent Change;

        EV_SET(&Change, FD, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, Tag); /* EV_CLEAR = edge triggered */
        
        if(kevent(Internal.Queue, &Change, 1, 0, 0, 0) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return;
        }

        EV_SET(&Change, FD, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, Tag);
        
        if(kevent(Internal.Queue, &Change, 1, 0, 0, 0) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return;
        }

    #endif
}

