
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

#ifndef LacewingEventPump
#define LacewingEventPump

#define SigRemoveClient        (void *) 0
#define SigEndWatcherThread    (void *) 1

struct EventPumpInternal
{  
    Lacewing::EventPump &Pump;
    ThreadTracker Threads;

    HANDLE CompletionPort;

    EventPumpInternal(Lacewing::EventPump &_Pump) : Pump(_Pump)
    {
        CompletionPort     = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 3, 0);
        HandlerTickNeeded  = 0;
    }

    inline void * Add (HANDLE Handle, void * Tag, void * Callback)
    {
        Event &E = EventBacklog.Borrow(*this);

        E.Tag      = Tag;
        E.Callback = Callback;
        E.Removing = false;

        HANDLE Result = CreateIoCompletionPort(Handle, CompletionPort, (ULONG_PTR) &E, 0);

        LacewingAssert(Result != 0);

        return &E;
    }

    inline void Remove (void * RemoveKey)
    {
        ((Event *) RemoveKey)->Removing = true;
        Pump.Post(SigRemoveClient, RemoveKey);
    }

    struct Event
    {
        Event(EventPumpInternal &)
        {
        }

        void * Tag;
        void * Callback;
        
        bool Removing;
    };

    Backlog<EventPumpInternal, Event> EventBacklog;

    void (LacewingHandler * HandlerTickNeeded) (Lacewing::EventPump &EventPump);

    OVERLAPPED * WatcherOverlapped;
    unsigned int WatcherBytesTransferred;
    EventPumpInternal::Event * WatcherEvent;
    Lacewing::Event WatcherResumeEvent;
    int WatcherError;
};

typedef EventPumpInternal PumpInternal;

#endif


