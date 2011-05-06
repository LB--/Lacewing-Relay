
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
    Lacewing::EventPump &EventPump;
    ThreadTracker Threads;

    int Queue;
    int PostFD_Read, PostFD_Write;

    EventPumpInternal(Lacewing::EventPump &_EventPump, int MaxHint) : EventPump(_EventPump)
    {
        #ifdef LacewingUseEPoll
            Queue = epoll_create(MaxHint);
        #endif

        #ifdef LacewingUseKQueue
            Queue = kqueue();
        #endif

        int PostPipe[2];
        pipe(PostPipe);
        
        PostFD_Read  = PostPipe[0];
        PostFD_Write = PostPipe[1];

        AddRead (PostFD_Read, 0, 0);
    }

    void * AddRead      (int FD, void * Tag, void * Callback);
    void * AddReadWrite (int FD, void * Tag, void * ReadCallback, void * WriteCallback);
    
    inline void Remove (void * RemoveKey)
    {
        ((Event *) RemoveKey)->Removing = true;
        EventPump.Post(SigRemoveClient, RemoveKey);
    }

    struct Event
    {
        Event(EventPumpInternal &)
        {
        }

        void * Tag, * ReadCallback, * WriteCallback;

        bool Removing;
    };

    Backlog<EventPumpInternal, Event> EventBacklog;

    queue<Event *> PostQueue;
    Lacewing::Sync Sync_PostQueue;
};

#endif


