
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

Lacewing::EventPump::EventPump(int MaxHint)
{
    InternalTag = new EventPumpInternal(*this, MaxHint);
    Tag         = 0;
}

Lacewing::EventPump::~EventPump()
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    delete ((EventPumpInternal *) InternalTag);
}

void Process(EventPumpInternal &Internal, EventPumpInternal::Event * Event, bool Gone, bool CanRead, bool CanWrite)
{
    if((!Event->ReadCallback) && (!Event->WriteCallback))
    {
        /* Post() */

        {   Lacewing::Sync::Lock Lock(Internal.Sync_PostQueue);

            while(!Internal.PostQueue.empty())
            {
                Event = Internal.PostQueue.front();
                Internal.PostQueue.pop();

                ((void (*) (void *)) Event->ReadCallback) (Event->Tag);

                Internal.EventBacklog.Return(*Event);
            }
        }

        return;
    }

    if(CanWrite)
    {
        if(!Event->Removing)
            ((void (*) (void *)) Event->WriteCallback) (Event->Tag);
    }
    
    if(CanRead)
    {
        if(Event->ReadCallback == SigRemoveClient)
            Internal.EventBacklog.Return(*(EventPumpInternal::Event *) Event->Tag);
        else
        {
            if(!Event->Removing)
                ((void (*) (void *, bool)) Event->ReadCallback) (Event->Tag, Gone);
        }
    }

    if(Gone)
        Event->Removing = true;
}


const int MaxEvents = 32;

Lacewing::Error * Lacewing::EventPump::Tick()
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    #ifdef LacewingUseEPoll
    
        epoll_event * EPollEvents = (epoll_event *) alloca(sizeof(epoll_event) * MaxEvents);
        int Count = epoll_wait(Internal.Queue, EPollEvents, MaxEvents, 0);
    
        for(int i = 0; i < Count; ++ i)
        {
            epoll_event &EPollEvent = EPollEvents[i];

            Process(Internal, (EventPumpInternal::Event *) EPollEvent.data.ptr,
                (EPollEvent.events & EPOLLRDHUP) != 0 || (EPollEvent.events & EPOLLHUP) != 0,
                    (EPollEvent.events & EPOLLIN) != 0, (EPollEvent.events & EPOLLOUT) != 0);
        }
   
    #endif
    
    #ifdef LacewingUseKQueue
    
        struct kevent * KEvents = (struct kevent *) alloca(sizeof(struct kevent) * MaxEvents);

        timespec Zero;
        memset(&Zero, 0, sizeof(Zero));
        
        int Count = kevent(Internal.Queue, 0, 0, KEvents, MaxEvents, &Zero);

        for(int i = 0; i < Count; ++ i)
        {
            struct kevent &KEvent = KEvents[i];

            Process(Internal, (EventPumpInternal::Event *) KEvent.udata, (KEvent.flags & EV_EOF) != 0,
                                KEvent.filter == EVFILT_READ, KEvent.filter == EVFILT_WRITE);
        }
        
    #endif

    return 0;
}

Lacewing::Error * Lacewing::EventPump::StartEventLoop()
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    for(;;)
    {
        #ifdef LacewingUseEPoll
        
            epoll_event * EPollEvents = (epoll_event *) alloca(sizeof(epoll_event) * MaxEvents);
            int Count = epoll_wait(Internal.Queue, EPollEvents, MaxEvents, -1);
        
            for(int i = 0; i < Count; ++ i)
            {
                epoll_event &EPollEvent = EPollEvents[i];

                Process(Internal, (EventPumpInternal::Event *) EPollEvent.data.ptr,
                    (EPollEvent.events & EPOLLRDHUP) != 0 || (EPollEvent.events & EPOLLHUP) != 0,
                        (EPollEvent.events & EPOLLIN) != 0, (EPollEvent.events & EPOLLOUT) != 0);
            }
       
        #endif
        
        #ifdef LacewingUseKQueue
        
            struct kevent * KEvents = (struct kevent *) alloca(sizeof(struct kevent) * MaxEvents);
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
                    Process(Internal, (EventPumpInternal::Event *) KEvent.udata,
                                    (KEvent.flags & EV_EOF) != 0, KEvent.filter == EVFILT_READ,
                                            KEvent.filter == EVFILT_WRITE);
                }
            }
            
        #endif
    }
        
    return 0;
}

void Lacewing::EventPump::Post(void * Function, void * Parameter)
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    EventPumpInternal::Event &Event = Internal.EventBacklog.Borrow(Internal);

    LacewingAssert(Function != 0);

    Event.ReadCallback  = Function;
    Event.WriteCallback = 0;
    Event.Tag           = Parameter;
    Event.Removing      = false;

    {   Lacewing::Sync::Lock Lock(Internal.Sync_PostQueue);
        Internal.PostQueue.push (&Event);
    }

    write(Internal.PostFD_Write, "", 1);
}

Lacewing::Error * Lacewing::EventPump::StartSleepyTicking(void (LacewingHandler * onTickNeeded) (Lacewing::EventPump &EventPump))
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    /* TODO */

    return 0;
}

void * EventPumpInternal::AddRead(int FD, void * Tag, void * Callback)
{
    Event &E = EventBacklog.Borrow(*this);

    E.Tag           = Tag;
    E.ReadCallback  = Callback;
    E.WriteCallback = 0;
    E.Removing      = false;

    #ifdef LacewingUseEPoll
    
        epoll_event Event;
        memset(&Event, 0, sizeof(epoll_event));

        Event.events = EPOLLIN | EPOLLET;
        Event.data.ptr = &E;

        if(epoll_ctl(Queue, EPOLL_CTL_ADD, FD, &Event) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return false;
        }
            
    #endif
    
    #ifdef LacewingUseKQueue
    
        struct kevent Change;
        EV_SET(&Change, FD, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, &E); /* EV_CLEAR = edge triggered */
        
        if(kevent(Queue, &Change, 1, 0, 0, 0) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return false;
        }
        
    #endif

    return &E;
}

void * EventPumpInternal::AddReadWrite(int FD, void * Tag, void * ReadCallback, void * WriteCallback)
{
    Event &E = EventBacklog.Borrow(*this);

    E.Tag            = Tag;
    E.ReadCallback   = ReadCallback;
    E.WriteCallback  = WriteCallback;
    E.Removing       = false;

    #ifdef LacewingUseEPoll
    
        epoll_event Event;
        memset(&Event, 0, sizeof(epoll_event));

        Event.events = EPOLLIN | EPOLLOUT | EPOLLET;
        Event.data.ptr = &E;

        if(epoll_ctl(Queue, EPOLL_CTL_ADD, FD, &Event) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return false;
        }
            
    #endif
    
    #ifdef LacewingUseKQueue
    
        struct kevent Change;

        EV_SET(&Change, FD, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, &E); /* EV_CLEAR = edge triggered */
        
        if(kevent(Queue, &Change, 1, 0, 0, 0) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return false;
        }

        EV_SET(&Change, FD, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, &E);
        
        if(kevent(Queue, &Change, 1, 0, 0, 0) == -1)
        {
            DebugOut("EventPump: Failed to add FD: " << strerror(errno));
            return false;
        }

    #endif

    return &E;
}

