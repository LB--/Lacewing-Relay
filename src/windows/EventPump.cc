
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

Lacewing::EventPump::EventPump(int)
{
    InternalTag = new EventPumpInternal(*this);
    Tag         = 0;
}

Lacewing::EventPump::~EventPump()
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    if(Internal.Threads.Living())
    {
        Post(SigEndWatcherThread, 0);

        Internal.WatcherResumeEvent.Signal();
        Internal.Threads.WaitUntilDead();
    }

    delete ((EventPumpInternal *) InternalTag);
}

inline void Process(EventPumpInternal &Internal, OVERLAPPED * Overlapped, unsigned int BytesTransferred, EventPumpInternal::Event &Event, int Error)
{
    if(Event.Callback == SigRemoveClient)
        Internal.EventBacklog.Return(*(EventPumpInternal::Event *) Event.Tag);
    else
    {
        if(!Event.Removing)
            ((void (*) (void *, OVERLAPPED *, unsigned int, int)) Event.Callback) (Event.Tag, Overlapped, BytesTransferred, Error);
    }

    if(Overlapped == (OVERLAPPED *) 1)
    {
        /* Event was created by Post() */

        Internal.EventBacklog.Return(Event);
    }
}

Lacewing::Error * Lacewing::EventPump::Tick()
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    if(Internal.HandlerTickNeeded)
    {
        /* Process whatever the watcher thread dequeued before telling the caller to tick */

        Process(Internal, Internal.WatcherOverlapped, Internal.WatcherBytesTransferred,
                    *Internal.WatcherEvent, Internal.WatcherError); 
    }

    OVERLAPPED * Overlapped;
    unsigned int BytesTransferred;
    
    EventPumpInternal::Event * Event;

    for(;;)
    {
        int Error = 0;

        if(!GetQueuedCompletionStatus(Internal.CompletionPort, (DWORD *) &BytesTransferred,
                 (PULONG_PTR) &Event, &Overlapped, 0))
        {
            Error = GetLastError();

            if(Error == WAIT_TIMEOUT)
                break;

            if(!Overlapped)
                break;
        }

        Process(Internal, Overlapped, BytesTransferred, *Event, Error);
    }
 
    if(Internal.HandlerTickNeeded)
        Internal.WatcherResumeEvent.Signal();

    return 0;
}

Lacewing::Error * Lacewing::EventPump::StartEventLoop()
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    OVERLAPPED * Overlapped;
    unsigned int BytesTransferred;
    
    EventPumpInternal::Event * Event;

    for(;;)
    {
        /* TODO : Use GetQueuedCompletionStatusEx where available */

        int Error = 0;

        if(!GetQueuedCompletionStatus(Internal.CompletionPort, (DWORD *) &BytesTransferred,
                 (PULONG_PTR) &Event, &Overlapped, INFINITE))
        {
            Error = GetLastError();

            if(!Overlapped)
                continue;
        }

        Process(Internal, Overlapped, BytesTransferred, *Event, Error);
    }
        
    return 0;
}

void Lacewing::EventPump::Post(void * Function, void * Parameter)
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    EventPumpInternal::Event &Event = Internal.EventBacklog.Borrow(Internal);

    Event.Callback = Function;
    Event.Tag      = Parameter;

    PostQueuedCompletionStatus(Internal.CompletionPort, 0, (ULONG_PTR) &Event, (OVERLAPPED *) 1);
}

LacewingThread(TickNeededWatcher, EventPumpInternal, Internal)
{
    for(;;)
    {
        Internal.WatcherError = 0;

        if(!GetQueuedCompletionStatus(Internal.CompletionPort, (DWORD *) &Internal.WatcherBytesTransferred,
                 (PULONG_PTR) &Internal.WatcherEvent, &Internal.WatcherOverlapped, INFINITE))
        {
            if((Internal.WatcherError = GetLastError()) == WAIT_TIMEOUT)
                break;

            if(!Internal.WatcherOverlapped)
                break;

            Internal.WatcherBytesTransferred = 0;
        }

        if(Internal.WatcherEvent->Callback == SigEndWatcherThread)
            break;

        Internal.HandlerTickNeeded(Internal.EventPump);

        Internal.WatcherResumeEvent.Wait();
        Internal.WatcherResumeEvent.Unsignal();
    }
}

Lacewing::Error * Lacewing::EventPump::StartSleepyTicking(void (LacewingHandler * onTickNeeded) (Lacewing::EventPump &EventPump))
{
    EventPumpInternal &Internal = *((EventPumpInternal *) InternalTag);

    Internal.HandlerTickNeeded = onTickNeeded;    
    Internal.Threads.Start(TickNeededWatcher, &Internal);

    return 0;
}


