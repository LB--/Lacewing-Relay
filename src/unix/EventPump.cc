
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

#include "../Common.h"

const char SigExitEventLoop  = 1;
const char SigRemove         = 2;
const char SigPost           = 3;

EventPump::Internal::Internal
    (Lacewing::EventPump &_EventPump, int MaxHint)
        : EventPump (_EventPump)
{
    {   int Pipe [2];
        pipe (Pipe);
    
        SignalPipe_Read   = Pipe [0];
        SignalPipe_Write  = Pipe [1];

        fcntl (SignalPipe_Read, F_SETFL, fcntl (SignalPipe_Read, F_GETFL, 0) | O_NONBLOCK);
    }

    #if defined (LacewingUseEPoll)

        Queue = epoll_create (MaxHint);

        {   epoll_event Event;
            memset (&Event, 0, sizeof (epoll_event));

            Event.events = EPOLLIN | EPOLLET;

            epoll_ctl (Queue, EPOLL_CTL_ADD, SignalPipe_Read, &Event);
        }

    #elif defined (LacewingUseKQueue)

        Queue = kqueue ();
        
        {   struct kevent Change;

            EV_SET (&Change, SignalPipe_Read, EVFILT_READ,
                EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, 0);

            kevent (Queue, &Change, 1, 0, 0, 0);
        }

    #endif
}

bool EventPump::Internal::Ready
    (Pump::Watch * watch, bool ReadReady, bool WriteReady)
{
    if (watch)
    {
        if (ReadReady && watch->onReadReady)
            watch->onReadReady (watch->Tag);

        if (WriteReady && watch->onWriteReady)
            watch->onWriteReady (watch->Tag);

        return true;
    }

    /* A null event means it must be the signal pipe */

    Lacewing::Sync::Lock Lock (Sync_Signals);

    char Signal;
    
    if (read (SignalPipe_Read, &Signal, sizeof (Signal)) == -1)
        return true;

    switch (Signal)
    {
        case SigExitEventLoop:
            return false;

        case SigRemove:
        {
            Pump::Watch * ToRemove = (Pump::Watch *) (** SignalParams.First);
            SignalParams.PopFront ();

            WatchBacklog.Return (*ToRemove);

            this->EventPump.RemoveUser ();

            break;
        }

        case SigPost:
        {
            void * Function = (** SignalParams.First);
            SignalParams.PopFront ();

            void * Parameter = (** SignalParams.First);
            SignalParams.PopFront ();

            ((void * (*) (void *)) Function) (Parameter);
            
            break;
        }
    };

    return true;
}

EventPump::EventPump (int MaxHint)
{
    internal = new EventPump::Internal (*this, MaxHint);
    Tag = 0;
}

EventPump::~EventPump ()
{
    delete internal;
}

const int MaxEvents = 16;

Error * EventPump::Tick ()
{
    #if defined (LacewingUseEPoll)

        epoll_event EPollEvents [MaxEvents];
        int Count = epoll_wait (internal->Queue, EPollEvents, MaxEvents, 0);
    
        for (int i = 0; i < Count; ++ i)
        {
            epoll_event &EPollEvent = EPollEvents [i];

            internal->Ready
            (
                (Pump::Watch *) EPollEvent.data.ptr,
                
                (EPollEvent.events & EPOLLIN) != 0
                    || (EPollEvent.events & EPOLLHUP) != 0
                    || (EPollEvent.events & EPOLLRDHUP) != 0,

                (EPollEvent.events & EPOLLOUT) != 0
            );
        }
   
    #elif defined (LacewingUseKQueue)
    
        struct kevent KEvents [MaxEvents];

        timespec Zero;
        memset (&Zero, 0, sizeof (Zero));
        
        int Count = kevent (internal->Queue, 0, 0, KEvents, MaxEvents, &Zero);

        for (int i = 0; i < Count; ++ i)
        {
            struct kevent &KEvent = KEvents [i];

            if (KEvent.filter == EVFILT_TIMER)
            {
                ((Lacewing::Timer *) KEvent.udata)->ForceTick ();
            }
            else
            {
                internal->Ready
                (
                    (Pump::Watch *) KEvent.udata,
                    
                    KEvent.filter == EVFILT_READ
                        || (KEvent.flags & EV_EOF),
                    
                    KEvent.filter == EVFILT_WRITE
                );
            }
        }
        
    #endif

    return 0;
}

Error * EventPump::StartEventLoop ()
{
    for (;;)
    {
        #if defined (LacewingUseEPoll)

            epoll_event EPollEvents [MaxEvents];
            int Count = epoll_wait (internal->Queue, EPollEvents, MaxEvents, -1);
        
            for (int i = 0; i < Count; ++ i)
            {
                epoll_event &EPollEvent = EPollEvents [i];

                if (!internal->Ready
                (
                    (Pump::Watch *) EPollEvent.data.ptr,
                    
                    (EPollEvent.events & EPOLLIN) != 0
                        || (EPollEvent.events & EPOLLHUP) != 0
                        || (EPollEvent.events & EPOLLRDHUP) != 0,

                    (EPollEvent.events & EPOLLOUT) != 0
                ))
                    break;
            }
       
        #elif defined (LacewingUseKQueue)
        
            struct kevent KEvents [MaxEvents];

            int Count = kevent (internal->Queue, 0, 0, KEvents, MaxEvents, 0);

            for (int i = 0; i < Count; ++ i)
            {
                struct kevent &KEvent = KEvents [i];

                if (KEvent.filter == EVFILT_TIMER)
                {
                    ((Lacewing::Timer *) KEvent.udata)->ForceTick ();
                }
                else
                {
                    if (!internal->Ready
                    (
                        (Pump::Watch *) KEvent.udata,
                        
                        KEvent.filter == EVFILT_READ
                            || (KEvent.flags & EV_EOF),
                        
                        KEvent.filter == EVFILT_WRITE
                    ))
                        break;
                }
            }
            
        #endif
    }
        
    return 0;
}

Error * EventPump::StartSleepyTicking
    (void (LacewingHandler * onTickNeeded) (EventPump &EventPump))
{
    /* TODO */

    return 0;
}

Pump::Watch * EventPump::Add (int FD, void * tag, Pump::Callback onReadReady,
                            Pump::Callback onWriteReady, bool edge_triggered)
{
    if ((!onReadReady) && (!onWriteReady))
        return 0;

    Pump::Watch &watch = internal->WatchBacklog.Borrow ();
    memset (&watch, 0, sizeof (Pump::Watch));

    watch.FD = FD;

    #if defined (LacewingUseEPoll)
    
        watch.onReadReady = onReadReady;
        watch.onWriteReady = onWriteReady;
        watch.EdgeTriggered = edge_triggered;
        watch.Tag = tag;

        epoll_event event;
        memset (&event, 0, sizeof (event));

        event.data.ptr = &watch;
        
        event.events = (onReadReady ? EPOLLIN : 0) |
                            (onWriteReady ? EPOLLOUT : 0) |
                                (edge_triggered ? EPOLLET : 0);

        epoll_ctl (internal->Queue, EPOLL_CTL_ADD, FD, &event);

    #elif defined (LacewingUseKQueue)

        UpdateCallbacks (&watch, tag, onReadReady,
                            onWriteReady, edge_triggered);

    #endif

    AddUser ();

    return &watch;
}

void EventPump::UpdateCallbacks
    (Pump::Watch * watch, void * tag, Callback onReadReady,
            Callback onWriteReady, bool edge_triggered)
{
    if ( ((onReadReady != 0) != (watch->onReadReady != 0))
            || ((onWriteReady != 0) != (watch->onWriteReady != 0))
                || (edge_triggered != watch->EdgeTriggered) )
    {
        #if defined (LacewingUseEPoll)
        
            epoll_event event;
            memset (&event, 0, sizeof (event));

            event.data.ptr = watch;

            event.events = (onReadReady ? EPOLLIN : 0) |
                                (onWriteReady ? EPOLLOUT : 0) |
                                    (edge_triggered ? EPOLLET : 0);

            epoll_ctl (internal->Queue, EPOLL_CTL_MOD, watch->FD, &event);

        #elif defined (LacewingUseKQueue)
        
            struct kevent change;

            if (onReadReady)
            {
                EV_SET (&change, watch->FD, EVFILT_READ,
                            EV_ADD | EV_ENABLE | EV_EOF |
                               (edge_triggered ? EV_CLEAR : 0), 0, 0, watch);

                kevent (internal->Queue, &change, 1, 0, 0, 0);
            }

            if (onWriteReady)
            {
                EV_SET (&change, watch->FD, EVFILT_WRITE,
                            EV_ADD | EV_ENABLE | EV_EOF |
                               (edge_triggered ? EV_CLEAR : 0), 0, 0, watch);

                kevent (internal->Queue, &change, 1, 0, 0, 0);
            }

        #endif
    }

    watch->onReadReady = onReadReady;
    watch->onWriteReady = onWriteReady;
    watch->EdgeTriggered = edge_triggered;
    watch->Tag = tag;
}

bool EventPump::IsEventPump ()
{
    return true;
}

void EventPump::Remove (Pump::Watch * watch)
{
    /* TODO : Should this remove the FD from epoll/kqueue immediately? */

    watch->onReadReady = 0;
    watch->onWriteReady = 0;

    Lacewing::Sync::Lock Lock (internal->Sync_Signals);

    internal->SignalParams.Push (watch);

    write (internal->SignalPipe_Write, &SigRemove, sizeof (SigRemove));
}

void EventPump::PostEventLoopExit ()
{
    Lacewing::Sync::Lock Lock (internal->Sync_Signals);

    write (internal->SignalPipe_Write, &SigExitEventLoop, sizeof (SigExitEventLoop));
}

void EventPump::Post (void * Function, void * Parameter)
{
    Lacewing::Sync::Lock Lock (internal->Sync_Signals);

    internal->SignalParams.Push (Function);
    internal->SignalParams.Push (Parameter);

    write (internal->SignalPipe_Write, &SigPost, sizeof (SigPost));
}

