
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

#include "Common.h"

const long WriterPresent = 0x8000;

struct SpinSyncInternal
{
    SpinSyncInternal()
    {
        Readers        = 0;
        WritersWaiting = 0;
    }

    ~SpinSyncInternal()
    {
    }

    volatile long Readers;
    volatile long WritersWaiting;
};

Lacewing::SpinSync::SpinSync()
{
    InternalTag = new SpinSyncInternal;
    Tag         = 0;
}

Lacewing::SpinSync::~SpinSync()
{
    delete ((SpinSyncInternal *) InternalTag);
}

Lacewing::SpinSync::ReadLock::ReadLock(Lacewing::SpinSync &Object)
{
    if(!(InternalTag = Object.InternalTag))
        return;

    SpinSyncInternal &Internal = *((SpinSyncInternal *) InternalTag);

    for(;;)
    {
        if(Internal.WritersWaiting)
        {
            LacewingYield();
            continue;
        }

        long OldReaders = Internal.Readers;

        if(OldReaders == WriterPresent)
        {
            LacewingYield();
            continue;
        }

        if(LacewingSyncCompareExchange((volatile LONG *) &Internal.Readers, OldReaders + 1, OldReaders) == OldReaders)
            break;
    }
}

Lacewing::SpinSync::ReadLock::~ReadLock()
{
    Release();
}

void Lacewing::SpinSync::ReadLock::Release()
{
    if(!InternalTag)
        return;

    LacewingSyncDecrement((volatile LONG *) &((SpinSyncInternal *) InternalTag)->Readers);
    InternalTag = 0;
}

Lacewing::SpinSync::WriteLock::WriteLock(Lacewing::SpinSync &Object)
{
    if(!(InternalTag = Object.InternalTag))
        return;

    SpinSyncInternal &Internal = *((SpinSyncInternal *) InternalTag);

    LacewingSyncIncrement(&Internal.WritersWaiting);

    for(;;)
    {
        if(Internal.Readers != 0)
        {
            LacewingYield();
            continue;
        }

        if(LacewingSyncCompareExchange((volatile LONG *) &Internal.Readers, WriterPresent, 0) == 0)
            break;
    }

    LacewingSyncDecrement(&Internal.WritersWaiting);
}

Lacewing::SpinSync::WriteLock::~WriteLock()
{
    Release();
}

void Lacewing::SpinSync::WriteLock::Release()
{
    if(!InternalTag)
        return;

    LacewingSyncExchange(&((SpinSyncInternal *) InternalTag)->Readers, 0);
    InternalTag = 0;
}

