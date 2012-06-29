
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
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

#include "../lw_common.h"

#define SigExitEventLoop     ((OVERLAPPED *) 1)
#define SigRemove            ((OVERLAPPED *) 2)
#define SigEndWatcherThread  ((OVERLAPPED *) 3)

struct Pump::Watch
{
    Callback onCompletion;

    void * Tag;
};

struct EventPump::Internal
{  
    Lacewing::EventPump &Pump;
    Lacewing::Thread WatcherThread;

    HANDLE CompletionPort;

    Internal (Lacewing::EventPump &_Pump)
        : Pump (_Pump), WatcherThread ("Watcher", (void *) Watcher)
    {
        CompletionPort     = CreateIoCompletionPort (INVALID_HANDLE_VALUE, 0, 4, 0);
        HandlerTickNeeded  = 0;
    }

    void (LacewingHandler * HandlerTickNeeded) (Lacewing::EventPump &EventPump);

    static bool Process (EventPump::Internal * internal, OVERLAPPED * Overlapped,
                            unsigned int BytesTransferred, Pump::Watch * Watch, int Error)
    {
        if (BytesTransferred == 0xFFFFFFFF)
        {
            /* See Post () */

            ((void * (*) (void *)) Watch) (Overlapped);

            return true;
        }

        if (Overlapped == SigRemove)
        {
            delete Watch;
            internal->Pump.RemoveUser ();

            return true;
        }

        if (Overlapped == SigExitEventLoop)
            return false;

        if (Watch->onCompletion)
            Watch->onCompletion (Watch->Tag, Overlapped, BytesTransferred, Error);

        return true;
    }
    
    OVERLAPPED * WatcherOverlapped;
    unsigned int WatcherBytesTransferred;
    EventPump::Pump::Watch * WatcherEvent;
    Lacewing::Event WatcherResumeEvent;
    int WatcherError;

    static void Watcher (EventPump::Internal * internal)
    {
        for (;;)
        {
            internal->WatcherError = 0;

            if (!GetQueuedCompletionStatus (internal->CompletionPort, (DWORD *) &internal->WatcherBytesTransferred,
                     (PULONG_PTR) &internal->WatcherEvent, &internal->WatcherOverlapped, INFINITE))
            {
                if ((internal->WatcherError = GetLastError ()) == WAIT_TIMEOUT)
                    break;

                if (!internal->WatcherOverlapped)
                    break;

                internal->WatcherBytesTransferred = 0;
            }

            if (internal->WatcherOverlapped == SigEndWatcherThread)
                break;

            internal->HandlerTickNeeded (internal->Pump);

            internal->WatcherResumeEvent.Wait ();
            internal->WatcherResumeEvent.Unsignal ();
        }
    }
};

EventPump::EventPump (int)
{
    lwp_init ();
    
    internal = new EventPump::Internal (*this);
    Tag = 0;
}

EventPump::~EventPump ()
{
    if (internal->WatcherThread.Started ())
    {
        PostQueuedCompletionStatus (internal->CompletionPort, 0, 0, SigEndWatcherThread);

        internal->WatcherResumeEvent.Signal ();
        internal->WatcherThread.Join ();
    }

    delete internal;
}

Error * EventPump::Tick ()
{
    if (internal->HandlerTickNeeded)
    {
        /* Process whatever the watcher thread dequeued before telling the caller to tick */

        Internal::Process (internal, internal->WatcherOverlapped, internal->WatcherBytesTransferred,
                            internal->WatcherEvent, internal->WatcherError); 
    }

    OVERLAPPED * Overlapped;
    unsigned int BytesTransferred;
    
    Pump::Watch * Watch;

    for(;;)
    {
        int Error = 0;

        if (!GetQueuedCompletionStatus
                (internal->CompletionPort, (DWORD *) &BytesTransferred,
                     (PULONG_PTR) &Watch, &Overlapped, 0))
        {
            Error = GetLastError();

            if(Error == WAIT_TIMEOUT)
                break;

            if(!Overlapped)
                break;
        }

        Internal::Process (internal, Overlapped, BytesTransferred, Watch, Error);
    }
 
    if (internal->HandlerTickNeeded)
        internal->WatcherResumeEvent.Signal ();

    return 0;
}

Error * EventPump::StartEventLoop ()
{
    OVERLAPPED * Overlapped;
    unsigned int BytesTransferred;
    
    Pump::Watch * Watch;

    bool Exit = false;

    for(;;)
    {
        /* TODO : Use GetQueuedCompletionStatusEx where available */

        int Error = 0;

        if (!GetQueuedCompletionStatus
                (internal->CompletionPort, (DWORD *) &BytesTransferred,
                        (PULONG_PTR) &Watch, &Overlapped, INFINITE))
        {
            Error = GetLastError();

            if(!Overlapped)
                continue;
        }

        if (!Internal::Process (internal, Overlapped, BytesTransferred, Watch, Error))
            break;
    }
        
    return 0;
}

void EventPump::PostEventLoopExit ()
{
    PostQueuedCompletionStatus (internal->CompletionPort, 0, 0, SigExitEventLoop);
}

Error * EventPump::StartSleepyTicking (void (LacewingHandler * onTickNeeded) (EventPump &EventPump))
{
    internal->HandlerTickNeeded = onTickNeeded;    
    internal->WatcherThread.Start (internal);

    return 0;
}

Pump::Watch * EventPump::Add (HANDLE Handle, void * Tag, Pump::Callback Callback)
{
    assert (Callback != 0);

    Pump::Watch * watch = new (std::nothrow) Pump::Watch;

    if (!watch)
        return 0;

    watch->onCompletion = Callback;
    watch->Tag = Tag;

    if (CreateIoCompletionPort (Handle,
            internal->CompletionPort, (ULONG_PTR) watch, 0) != 0)
    {
        /* success */
    }
    else
    {
        assert (false);
    }
    
    AddUser ();

    return watch;
}

void EventPump::Remove (Pump::Watch * Watch)
{
    Watch->onCompletion = 0;

    PostQueuedCompletionStatus
        (internal->CompletionPort, 0, (ULONG_PTR) Watch, SigRemove);
}

bool EventPump::IsEventPump ()
{
    return true;
}

void EventPump::Post (void * Function, void * Parameter)
{
    PostQueuedCompletionStatus
        (internal->CompletionPort, 0xFFFFFFFF,
            (ULONG_PTR) Function, (OVERLAPPED *) Parameter);
}

void EventPump::UpdateCallbacks
    (Pump::Watch * watch, void * tag, Pump::Callback onCompletion)
{
    watch->Tag = tag;
    watch->onCompletion = onCompletion;

}

