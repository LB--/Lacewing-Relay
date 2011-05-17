
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

#ifndef LacewingPump
#define LacewingPump

#define SigRemoveClient        (void *) 0
#define SigEndWatcherThread    (void *) 1

struct PumpInternal
{
    Lacewing::Pump &Pump;

    int PostFD_Read, PostFD_Write;
    bool PostFD_Added;

    PumpInternal (Lacewing::Pump &_Pump);
    
    void * AddRead      (int FD, void * Tag, void * Callback);
    void * AddReadWrite (int FD, void * Tag, void * ReadCallback, void * WriteCallback);

    struct Event
    {
        Event(PumpInternal &)
        {
        }

        void * Tag, * ReadCallback, * WriteCallback;
        bool Removing;
    };

    inline void Remove (void * RemoveKey)
    {
        ((Event *) RemoveKey)->Removing = true;
        Pump.Post(SigRemoveClient, RemoveKey);
    }
    
    Backlog<PumpInternal, Event> EventBacklog;

    queue<Event *> PostQueue;
    Lacewing::Sync Sync_PostQueue;

    bool IsEventPump;

    friend struct Lacewing::Pump;
};

#endif


