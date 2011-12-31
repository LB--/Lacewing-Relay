
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "Common.h"

struct Sync::Internal
{
    Internal ()
    {
        #ifdef LacewingWindows  
            
            InitializeCriticalSection(&CriticalSection);

        #else

            pthread_mutexattr_init(&Attributes);
            pthread_mutexattr_settype(&Attributes, PTHREAD_MUTEX_RECURSIVE);

            pthread_mutex_init(&Mutex, &Attributes);

        #endif
    }

    ~ Internal ()
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

Sync::Sync ()
{
    internal = new Sync::Internal;
    Tag = 0;
}

Sync::~Sync ()
{
    delete internal;
}

Sync::Lock::Lock (Sync &Object)
{
    if (!(internal = Object.internal))
        return;

    #ifdef LacewingWindows
        EnterCriticalSection (&((Sync::Internal *) internal)->CriticalSection);
    #else
        pthread_mutex_lock (&((Sync::Internal *) internal)->Mutex);
    #endif
}

Sync::Lock::~Lock ()
{
    Release ();
}

void Sync::Lock::Release ()
{
    if (!internal)
        return;

    #ifdef LacewingWindows
        LeaveCriticalSection (&((Sync::Internal *)internal)->CriticalSection);
    #else
        pthread_mutex_unlock (&((Sync::Internal *) internal)->Mutex);
    #endif

    internal = 0;
}

