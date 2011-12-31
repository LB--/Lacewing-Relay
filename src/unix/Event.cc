
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

#include "../Common.h"

struct Event::Internal
{
    Lacewing::Event &Event;

    int PipeR, PipeW;

    Internal (Lacewing::Event &_Event)
        : Event (_Event)
    {
        int Pipe [2];
        pipe (Pipe);

        PipeR = Pipe [0];
        PipeW = Pipe [1];
        
        fcntl (PipeR, F_SETFL, fcntl (PipeR, F_GETFL, 0) | O_NONBLOCK);
    }

    ~ Internal ()
    {
        close (PipeW);
        close (PipeR);
    }
};

Event::Event ()
{
    internal = new Internal (*this);
    Tag = 0;
}

Event::~Event ()
{
    delete internal;
}

bool Event::Signalled ()
{
    fd_set Set;

    FD_ZERO (&Set);
    FD_SET (internal->PipeR, &Set);

    timeval Timeout = { 0 };

    return select (internal->PipeR + 1, &Set, 0, 0, &Timeout) > 0;
}

void Event::Signal ()
{
    write (internal->PipeW, "", 1);
}

void Event::Unsignal ()
{
    char buf [16];

    while (read (internal->PipeR, buf, sizeof (buf)) != -1)
    {
    }
}

bool Event::Wait (int Timeout)
{      
    fd_set Set;

    FD_ZERO (&Set);
    FD_SET (internal->PipeR, &Set);

    timeval tv;

    tv.tv_sec = Timeout / 1000;
    tv.tv_usec = (Timeout % 1000) * 1000;

    return select (internal->PipeR + 1, &Set, 0, 0, &tv) > 0;
}

