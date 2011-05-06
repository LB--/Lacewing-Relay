
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

struct SyncInternal
{
    SyncInternal()
    {
        #ifdef LacewingWindows  
            
            InitializeCriticalSection(&CriticalSection);

        #else

            pthread_mutexattr_init(&Attributes);
            pthread_mutexattr_settype(&Attributes, PTHREAD_MUTEX_RECURSIVE);

            pthread_mutex_init(&Mutex, &Attributes);

        #endif
    }

    ~SyncInternal()
    {
        #ifdef LacewingWindows

            DeleteCriticalSection(&CriticalSection);

        #else

            pthread_mutex_destroy(&Mutex);
            pthread_mutexattr_destroy(&Attributes);

        #endif
    }

    #ifdef LacewingWindows
    
        CRITICAL_SECTION CriticalSection;

    #else

        pthread_mutex_t Mutex;
        pthread_mutexattr_t Attributes;

    #endif
};

Lacewing::Sync::Sync()
{
    InternalTag = new SyncInternal;
    Tag         = 0;
}

Lacewing::Sync::~Sync()
{
    delete ((SyncInternal *) InternalTag);
}

Lacewing::Sync::Lock::Lock(Lacewing::Sync &Object)
{
    if(!(InternalTag = Object.InternalTag))
        return;

    #ifdef LacewingWindows
        EnterCriticalSection(&((SyncInternal *)InternalTag)->CriticalSection);
    #else
        pthread_mutex_lock(&((SyncInternal *) InternalTag)->Mutex);
    #endif
}

Lacewing::Sync::Lock::~Lock()
{
    Release();
}

void Lacewing::Sync::Lock::Release()
{
    if(!InternalTag)
        return;

    #ifdef LacewingWindows
        LeaveCriticalSection(&((SyncInternal *)InternalTag)->CriticalSection);
    #else
        pthread_mutex_unlock(&((SyncInternal *) InternalTag)->Mutex);
    #endif

    InternalTag = 0;
}

