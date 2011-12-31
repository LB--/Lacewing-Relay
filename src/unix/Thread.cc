
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

struct Thread::Internal
{
    void * Function, * Parameter;
    String Name;

    pthread_t Thread;
    bool Started;

    Internal (const char * _Name, void * _Function)
        : Function (_Function), Name (_Name)
    {
        Started = false;
    }
};

Thread::Thread (const char * Name, void * Function)
{
    internal = new Internal (Name, Function);
}

Thread::~Thread ()
{
    Join ();

    delete internal;
}

static int ThreadWrapper (Thread::Internal * internal)
{
    #if HAVE_DECL_PR_SET_NAME != 0
        prctl (PR_SET_NAME, (unsigned long) (const char *) internal->Name, 0, 0, 0);
    #endif
    
    int ExitCode = ((int (*) (void *)) internal->Function) (internal->Parameter);

    internal->Started = false;

    return ExitCode;
}

void Thread::Start (void * Parameter)
{
    if (Started ())
        return;

    internal->Parameter = Parameter;
    
    internal->Started = pthread_create
        (&internal->Thread, 0, (void * (*) (void *)) ThreadWrapper, internal);
}

bool Thread::Started ()
{
    return internal->Started;
}

int Thread::Join ()
{
    if (!Started ())
        return -1;

    void * ExitCode;

    if (pthread_join (internal->Thread, &ExitCode))
        return -1;
        
    return (int) (lw_iptr) ExitCode;
}


