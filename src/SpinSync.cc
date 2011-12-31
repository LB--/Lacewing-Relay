
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

const long WriterPresent = 0x8000;

struct SpinSync::Internal
{
    Internal ()
    {
        Readers        = 0;
        WritersWaiting = 0;
    }

    ~ Internal ()
    {
    }

    volatile long Readers;
    volatile long WritersWaiting;
};

SpinSync::SpinSync ()
{
    internal = new SpinSync::Internal;
    Tag = 0;
}

SpinSync::~SpinSync ()
{
    delete internal;
}

SpinSync::ReadLock::ReadLock (SpinSync &Object)
{
    if (!(internal = Object.internal))
        return;

    for (;;)
    {
        if (internal->WritersWaiting)
        {
            LacewingYield ();
            continue;
        }

        long OldReaders = internal->Readers;

        if(OldReaders == WriterPresent)
        {
            LacewingYield();
            continue;
        }

        if (LacewingSyncCompareExchange ((volatile long *) &internal->Readers, OldReaders + 1, OldReaders) == OldReaders)
            break;
    }
}

SpinSync::ReadLock::~ReadLock ()
{
    Release();
}

void SpinSync::ReadLock::Release ()
{
    if (!internal)
        return;

    LacewingSyncDecrement ((volatile long *) &internal->Readers);
    internal = 0;
}

SpinSync::WriteLock::WriteLock (SpinSync &Object)
{
    if (!(internal = Object.internal))
        return;

    LacewingSyncIncrement (&internal->WritersWaiting);

    for(;;)
    {
        if(internal->Readers != 0)
        {
            LacewingYield();
            continue;
        }

        if (LacewingSyncCompareExchange ((volatile long *) &internal->Readers, WriterPresent, 0) == 0)
            break;
    }

    LacewingSyncDecrement (&internal->WritersWaiting);
}

SpinSync::WriteLock::~WriteLock ()
{
    Release();
}

void SpinSync::WriteLock::Release ()
{
    if (!internal)
        return;

    LacewingSyncExchange (&internal->Readers, 0);
    internal = 0;
}

