
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

Lacewing::Pump::Pump()
{
    InternalTag = new PumpInternal(*this);
    Tag         = 0;
}

Lacewing::Pump::~Pump()
{
    delete ((PumpInternal *) InternalTag);
}

void Lacewing::Pump::Ready (void * Tag, bool Gone, bool CanRead, bool CanWrite)
{
    PumpInternal &Internal = *((PumpInternal *) InternalTag);

    PumpInternal::Event * Event = (PumpInternal::Event *) Tag;

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
            Internal.EventBacklog.Return(*(PumpInternal::Event *) Event->Tag);
        else
        {
            if(!Event->Removing)
                ((void (*) (void *, bool)) Event->ReadCallback) (Event->Tag, Gone);
        }
    }

    if(Gone)
        Event->Removing = true;
}

void Lacewing::Pump::Post (void * Function, void * Parameter)
{
    PumpInternal &Internal = *((PumpInternal *) InternalTag);

    if(!Internal.PostFD_Added)
        AddRead (Internal.PostFD_Read, 0);

    PumpInternal::Event &Event = Internal.EventBacklog.Borrow(Internal);

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

void * PumpInternal::AddRead (int FD, void * Tag, void * Callback)
{
    Event &E = EventBacklog.Borrow(*this);

    E.Tag           = Tag;
    E.ReadCallback  = Callback;
    E.WriteCallback = 0;
    E.Removing      = false;

    Pump.AddRead(FD, &E);
    return &E;
}

void * PumpInternal::AddReadWrite (int FD, void * Tag, void * ReadCallback, void * WriteCallback)
{
    Event &E = EventBacklog.Borrow(*this);

    E.Tag           = Tag;
    E.ReadCallback  = ReadCallback;
    E.WriteCallback = WriteCallback;
    E.Removing      = false;

    Pump.AddReadWrite(FD, &E);
    return &E;
}

PumpInternal::PumpInternal (Lacewing::Pump &_Pump) : Pump(_Pump)
{
    int PostPipe[2];
    pipe(PostPipe);
    
    PostFD_Read  = PostPipe[0];
    PostFD_Write = PostPipe[1];
    PostFD_Added = false;
}

