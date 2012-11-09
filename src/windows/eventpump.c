
/* vim: set et ts=3 sw=3 ft=c:
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

#include "../common.h"

#define sig_exit_event_loop      ((OVERLAPPED *) 1)
#define sig_remove               ((OVERLAPPED *) 2)
#define sig_end_watcher_thread   ((OVERLAPPED *) 3)

struct lw_pump_watch
{
    lw_pump_callback on_completion;

    void * tag;
};

struct lw_eventpump
{  
    struct lw_pump pump;

    lw_thread watcher_thread;

    HANDLE completion_port;

    void (lw_callback * on_tick_needed) (lw_eventpump);

    static bool Process (EventPump::Internal * ctx, OVERLAPPED * Overlapped,
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
            ctx->Pump.RemoveUser ();

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

    static void Watcher (EventPump::Internal * ctx)
    {
        for (;;)
        {
            ctx->WatcherError = 0;

            if (!GetQueuedCompletionStatus (ctx->CompletionPort, (DWORD *) &ctx->WatcherBytesTransferred,
                     (PULONG_PTR) &ctx->WatcherEvent, &ctx->WatcherOverlapped, INFINITE))
            {
                if ((ctx->WatcherError = GetLastError ()) == WAIT_TIMEOUT)
                    break;

                if (!ctx->WatcherOverlapped)
                    break;

                ctx->WatcherBytesTransferred = 0;
            }

            if (ctx->WatcherOverlapped == SigEndWatcherThread)
                break;

            ctx->HandlerTickNeeded (ctx->Pump);

            ctx->WatcherResumeEvent.Wait ();
            ctx->WatcherResumeEvent.Unsignal ();
        }
    }
};

lw_eventpump lw_eventpump_new ()
{
   lw_eventpump ctx = calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   lwp_init ();

   ctx->watcher_thread = lw_thread_new ("watcher", watcher);
   ctx->completion_port = CreateIoCompletionPort (INVALID_HANDLE_VALUE, 0, 4, 0);

   lwp_pump_init ((lw_pump) ctx);

   return ctx;
}

void lw_eventpump_delete (lw_eventpump ctx)
{
   if (lw_thread_started (ctx->watcher_thread))
   {
      PostQueuedCompletionStatus
         (ctx->completion_port, 0, 0, sig_end_watcher_thread);

      lw_event_signal (ctx->watcher_resume_event);
      lw_thread_join (ctx->watcher_thread);
   }

   free (ctx);
}

lw_error lw_eventpump_tick (lw_eventpump ctx)
{
   if (ctx->on_tick_needed)
   {
      /* Process whatever the watcher thread dequeued before telling the caller to tick */

      process (ctx, ctx->watcher_overlapped,
                    ctx->watcher_bytes_transferred,
                    ctx->watcher_event,
                    ctx->watcher_error); 
   }

   OVERLAPPED overlapped;
   DWORD bytes_transferred;

   lw_pump_watch watch;

   for (;;)
   {
      int error = 0;

      if (!GetQueuedCompletionStatus (ctx->completion_port,
               &bytes_transferred,
               (PULONG_PTR) &watch,
               &overlapped, 0))
      {
         error = GetLastError ();

         if (error == WAIT_TIMEOUT)
            break;

         if (!overlapped)
            break;
      }

      process (ctx, overlapped, bytes_transferred, watch, error);
   }

   if (ctx->handler_tick_needed)
      lw_event_signal (ctx->watcher_resume_event);

   return 0;
}

lw_error lw_eventpump_start_eventloop (lw_eventpump ctx)
{
   OVERLAPPED * Overlapped;
   DWORD BytesTransferred;

   lw_pump_watch watch;
   lw_bool exit = lw_false;

   for (;;)
   {
      /* TODO : Use GetQueuedCompletionStatusEx where available */

      int error = 0;

      if (!GetQueuedCompletionStatus (ctx->completion_port,
                                      &bytes_transferred,
                                      (PULONG_PTR) &watch,
                                      &overlapped, INFINITE))
      {
         error = GetLastError();

         if (!overlapped)
            continue;
      }

      if (!process (ctx, overlapped, bytes_transferred, watch, error))
         break;
   }

   return 0;
}

void lw_eventpump_post_eventloop_exit (lw_eventpump ctx)
{
    PostQueuedCompletionStatus (ctx->completion_port, 0, 0, sig_exit_event_loop);
}

lw_error lw_eventpump_start_sleepy_ticking
    (void (lw_callback * on_tick_needed) (lw_eventpump))
{
    ctx->on_tick_needed = on_tick_needed;    
    lw_thread_start (ctx->watcher_thread, ctx);

    return 0;
}

lw_pump_watch lw_eventpump_add (lw_eventpump ctx, HANDLE handle,
                                void * tag, lw_pump_callback callback)
{
    assert (callback != 0);

    Pump::Watch * watch = new (std::nothrow) Pump::Watch;

    if (!watch)
        return 0;

    watch->onCompletion = Callback;
    watch->Tag = Tag;

    if (CreateIoCompletionPort (Handle,
            ctx->CompletionPort, (ULONG_PTR) watch, 0) != 0)
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
        (ctx->CompletionPort, 0, (ULONG_PTR) Watch, SigRemove);
}

bool EventPump::IsEventPump ()
{
    return true;
}

void EventPump::Post (void * Function, void * Parameter)
{
    PostQueuedCompletionStatus
        (ctx->CompletionPort, 0xFFFFFFFF,
            (ULONG_PTR) Function, (OVERLAPPED *) Parameter);
}

void EventPump::UpdateCallbacks
    (Pump::Watch * watch, void * tag, Pump::Callback onCompletion)
{
    watch->Tag = tag;
    watch->onCompletion = onCompletion;

}

