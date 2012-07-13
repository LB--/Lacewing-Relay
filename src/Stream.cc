
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2012 James McLaughlin.  All rights reserved.
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

Stream::Stream ()
{
    internal = new Internal (this);

    internal->Graph->Expand ();
}

Stream::Stream (Lacewing::Pump &Pump)
{
    internal = new Internal (this);

    internal->Pump = &Pump;

    internal->Graph->Expand ();
}

Stream::~ Stream ()
{
    Close ();

    internal->DataHandlers.Clear ();

    internal->Graph->ClearExpanded ();
    internal->Graph->Roots.Remove (internal);

    /* Is the graph empty now? */

    if (internal->Graph->Roots.Size == 0)
        delete internal->Graph;
    else
        internal->Graph->Expand ();

    for (List <Internal::Filter>::Element * E =
            internal->FiltersUpstream.First; E; E = E->Next)
    {
        if ((** E).Owned)
            delete (** E).StreamPtr->Public;
    }

    for (List <Internal::Filter>::Element * E
            = internal->FiltersDownstream.First; E; E = E->Next)
    {
        if ((** E).Owned)
            delete (** E).StreamPtr->Public;
    }

    /* TODO : clear queues */

    internal->Public = 0;

    /* The stream has now been detached from everything, but if some other
     * routine is in progress, we'll leave the internal structure around
     * for that to delete later.
     */

    if (internal->UserCount == 0)
        delete internal;
}

Stream::Internal::Internal (Stream * _Public) : Public (_Public)
{
    UserCount = 0;

    Queueing = false;
    Closing = false;

    HeadUpstream = 0;
    PrevDirect = 0;

    Graph = new (std::nothrow) StreamGraph ();

    Graph->Roots.Push (this);

    LastExpand = Graph->LastExpand;
}

Stream::Internal::~ Internal ()
{
    if (Public)
        Public->Close ();
}

void Stream::Write (const char * buffer, size_t size)
{
    internal->Write (buffer, size, 0);
}

size_t Stream::WritePartial (const char * buffer, size_t size)
{
    return internal->Write (buffer, size, Internal::Write_Partial);
}

size_t Stream::Internal::Write (const char * buffer, size_t size, int flags)
{
    if (size == -1)
        size = strlen (buffer);

    if (size == 0)
        return size; /* nothing to do */

    if ((! (flags & Write_IgnoreFilters)) && HeadUpstream)
    {
        HeadUpstream->Write (buffer, size, 0);
        return size;
    }

    if (PrevExpanded.Size)
    {
        if (! (flags & Write_IgnoreBusy))
        {
            if (flags & Write_Partial)
                return 0;

            /* Something is behind us, but this data doesn't come from it.
             * Queue the data to write when we're not busy.
             */

            if (!BackQueue.Size)
                BackQueue.Push ();

            (** BackQueue.Last).Buffer.Add (buffer, size);

            return size;
        }

        /* Something is behind us and gave us this data. */

        if ((! (flags & Write_IgnoreQueue)) && FrontQueue.Size)
        {
            if (flags & Write_Partial)
                return 0;

            (** FrontQueue.Last).Buffer.Add (buffer, size);
            return size;
        }

        if (IsTransparent ())
        {
            Push (buffer, size);
            return size;
        }

        if ((! (flags & Write_IgnoreFilters)) && HeadUpstream)
        {
            HeadUpstream->Write (buffer, size, 0);
            return size;
        }

        size_t written = Public->Put (buffer, size);

        if (flags & Write_Partial)
            return written;

        if (written < size)
        {
            if (!FrontQueue.Size)
                FrontQueue.Push ();

            (** FrontQueue.Last).Buffer.Add
                    (buffer + written, size - written);
        }

        return size;
    }

    if ( (! (flags & Write_IgnoreQueue)) && (Queueing || BackQueue.Size))
    {
        if (flags & Write_Partial)
            return 0;

        if (!BackQueue.Size)
            BackQueue.Push ();

        (** BackQueue.Last).Buffer.Add (buffer, size);

        return size;
    }

    if (IsTransparent ())
    {
        Push (buffer, size);
        return size;
    }

    size_t written = Public->Put (buffer, size);

    if (flags & Write_Partial)
        return written;

    if (written < size)
    {
        if (!BackQueue.Size)
            BackQueue.Push ();

        if (flags & Write_IgnoreQueue)
        {
            Queued &first = (** BackQueue.First);

            if (!first.Buffer.Size)
                first.Buffer.Add (buffer + written, size - written);
            else
            {
                /* TODO : rewind offset where possible instead of creating a new Queued? */

                (** BackQueue.PushFront ()).Buffer.Add
                    (buffer + written, size - written);
            }
        }
        else
            (** BackQueue.Last).Buffer.Add (buffer + written, size - written);
    }

    return size;
}

void Stream::Write (Stream &stream, size_t size, bool delete_when_finished)
{
    internal->Write (stream.internal, size,
            delete_when_finished ? Internal::Write_DeleteStream : 0);
}

void Stream::Internal::Write (Stream::Internal * stream, size_t size, int flags)
{
    if ( (!(flags & Write_IgnoreQueue)) && (BackQueue.Size || Queueing))
    {
        Queued &queued = ** BackQueue.Push ();

        queued.StreamPtr = stream;
        queued.StreamBytesLeft = size;

        if (flags & Write_DeleteStream)
            queued.Flags |= Queued::Flag_DeleteStream;

        return;
    }

    /* Are we currently in a different graph from the source stream? */

    if (stream->Graph != Graph)
        stream->Graph->Swallow (Graph);

    LacewingAssert (stream->Graph == Graph);

    StreamGraph::Link * link = new (std::nothrow) StreamGraph::Link
                (stream, 0, this, 0, size, 0, flags & Write_DeleteStream);

    stream->Next.Push (link);
    Prev.Push (link);

    /* This stream is now linked to, so doesn't need to be a root */

    Graph->Roots.Remove (this);

    Graph->Expand ();
    Graph->Read ();
}

void Stream::WriteFile (const char * filename)
{
    /* This method may only be used when the stream is associated with a pump */

    LacewingAssert (internal->Pump);

    File * file = new File (*internal->Pump, filename, "r");

    if (!file->Valid ())
        return;

    Write (*file, file->BytesLeft (), true);
}

void Stream::AddFilterUpstream (Stream &filter, bool delete_with_stream)
{
    Internal::Filter _filter = { filter.internal, delete_with_stream };

    /* Upstream data passes through the most recently added filter first */

    internal->FiltersUpstream.PushFront (_filter);

    if (filter.internal->Graph != internal->Graph)
        internal->Graph->Swallow (filter.internal->Graph);

    internal->Graph->Expand ();
    internal->Graph->Read ();
}

void Stream::AddFilterDownstream (Stream &filter, bool delete_with_stream)
{
    Internal::Filter _filter = { filter.internal, delete_with_stream };

    /* Downstream data passes through the most recently added filter last */

    internal->FiltersDownstream.Push (_filter);

    if (filter.internal->Graph != internal->Graph)
        internal->Graph->Swallow (filter.internal->Graph);

    internal->Graph->Expand ();
    internal->Graph->Read ();
}

size_t Stream::BytesLeft ()
{
    return -1;
}

void Stream::Read (size_t bytes)
{
}

void Stream::Internal::Data (char * buffer, size_t size)
{
    ++ UserCount;

    switch (ExpDataHandlers.Size)
    {
        case 0:
            break;

        case 1:
        {
            /* If there's only one handler, there's no need to copy the data */

            DataHandler &handler = ** ExpDataHandlers.First;

            handler.Proc (*handler.StreamPtr->Public,
                                handler.Tag, buffer, size);

            /* Handler may destroy the stream */

            if (!Public)
            {
                delete this;
                return;
            }

            break;
        }

        default:
        {
            /* Otherwise, copy the data for each handler (thus allowing it to
             * be modified without consequence).  TODO : limit on size for
             * stack copy?
             */

            char * copy = (char *) alloca (size);

            for (List <DataHandler>::Element * E
                    = ExpDataHandlers.First; E; E = E->Next)
            {
                DataHandler &handler = (** E);

                memcpy (copy, buffer, size);

                handler.Proc (*handler.StreamPtr->Public,
                                    handler.Tag, copy, size);

                if (!Public)
                {
                    delete this;
                    return;
                }
            }

            break;
        }
    };

    /* Write the data to any streams next in the (expanded) graph */

    Push (buffer, size);

    -- UserCount;

    /* Pushing data may result in stream destruction */

    if (UserCount == 0 && !Public)
    {
        delete this;
        return;
    }

}

void Stream::Data (char * buffer, size_t size)
{
    internal->Data (buffer, size);
}

void Stream::Internal::Push (const char * buffer, size_t size)
{
    ++ Graph->LastPush;

PushLoop:

    int LastExpand = Graph->LastExpand;

    for (List <StreamGraph::Link *>::Element * E
                = NextExpanded.First; E; E = E->Next)
    {
        StreamGraph::Link * link = (** E);

        if (link->LastPush == Graph->LastPush)
            continue;

        link->LastPush = Graph->LastPush;

        size_t to_write = link->BytesLeft != -1 &&
                            size > link->BytesLeft ? link->BytesLeft : size;

        if (!link->ToExp->IsTransparent ())
        {
            /* If we have some actual data, write it to the target stream */

            if (buffer)
            {
                link->ToExp->Write
                    (buffer, to_write, Write_IgnoreFilters | Write_IgnoreBusy);
            }
        }
        else
        {
            /* Target stream is transparent - have it push the data forward */

            link->ToExp->Push (buffer, to_write);
        }

        /* Calling Write() or Push() on the next stream may have caused the
         * graph to re-expand (rendering our iterator invalid).
         */

        if (Graph->LastExpand != LastExpand)
            goto PushLoop;

        if (!link->To)
            continue; /* filters cannot expire */

        if (link->BytesLeft == -1)
        {
            if (link->FromExp->Public->BytesLeft () != 0)
                continue;
        }
        else
        {
            if ((link->BytesLeft -= to_write) > 0)
                continue;
        }

        if (link->DeleteStream)
        {
            delete link->From->Public; /* will re-expand the graph */

            break;
        }
        else
        {
            Graph->ClearExpanded ();

            Next.Remove (link);
            link->To->Prev.Remove (link);

            /* Since the target and anything after it are still part
             * of this graph, make it a root before deleting the link.
             */

            Graph->Roots.Push (link->To);

            Graph->Expand ();

            delete link;

            Graph->Read ();

            goto PushLoop;
        }
    }
}

size_t Stream::PutWritable (char * buffer, size_t size)
{
    return Put (buffer, size);
}

size_t Stream::Put (Stream &, size_t size)
{
    return -1;
}

void Stream::WriteReady ()
{
    if (!internal->WriteDirect ())
    {
        /* TODO: ??? */
    }

    internal->WriteQueued ();
}

void Stream::Internal::WriteQueued ()
{
    if (!Queueing)
    {
        WriteQueued (FrontQueue);

        if (FrontQueue.Size == 0 && !PrevExpanded.Size)
            WriteQueued (BackQueue);
    }
}

void Stream::Internal::WriteQueued (List <Queued> &queue)
{
    while (queue.Size)
    {
        Queued &queued = ** queue.First;

        if (queued.Flags & Queued::Flag_BeginQueue)
        {
            queue.PopFront ();
            Queueing = true;

            return;
        }

        if ((queued.Buffer.Size - queued.Buffer.Offset) > 0)
        {
            /* There's still something in the buffer that needs to be written */

            int written = Write (queued.Buffer.Buffer + queued.Buffer.Offset,
                                      queued.Buffer.Size - queued.Buffer.Offset,
                                            Write_IgnoreQueue | Write_Partial |
                                                  Write_IgnoreBusy);

            if ((queued.Buffer.Offset += written) < queued.Buffer.Size)
                break; /* couldn't write everything */

            queued.Buffer.Reset ();
        }

        if (!queued.StreamPtr)
        {
            queue.PopFront ();
            continue;
        }
        
        /* There's a Stream to write now */

        Stream::Internal * stream = queued.StreamPtr;
        size_t bytes = queued.StreamBytesLeft;

        int flags = Write_IgnoreQueue;

        if (queued.Flags & Queued::Flag_DeleteStream)
            flags |= Write_DeleteStream;

        queue.PopFront ();

        Write (stream, bytes, flags);
    }
}

bool Stream::Internal::WriteDirect ()
{
    if (!PrevDirect)
        return true;

    size_t written = Public->Put (*PrevDirect->Public, DirectBytesLeft);

    if (written != -1)
    {
        PrevDirect->Push (0, written);

        if (DirectBytesLeft != -1)
            DirectBytesLeft -= written;

        return true;
    }

    return false;
}

void Stream::Internal::Close ()
{
    if (Closing)
        return;

    Closing = true;

    ++ UserCount;


    Graph->ClearExpanded ();


    /* Anything that comes before us can no longer link here */

    for (List <StreamGraph::Link *>::Element * E = Prev.First;
                E; E = E->Next)
    {
        StreamGraph::Link * link = (** E);

        link->From->Next.Remove (link);

        delete link;
    }

    Prev.Clear ();


    /* Anything that comes after us will have to be a root */

    for (List <StreamGraph::Link *>::Element * E = Next.First;
                E; E = E->Next)
    {
        StreamGraph::Link * link = (** E);

        Graph->Roots.Push (link->To);

        link->To->Prev.Remove (link);

        delete link;
    }

    Next.Clear ();


    if (!Graph->Roots.Find (this))
        Graph->Roots.Push (this);

    Graph->Expand ();

    /* TODO : Should Read be called here? */


    /* Have to make a local copy of the close handlers as
     * a close handler may destroy the Stream.
     */

    List <Internal::CloseHandler> ToCall;

    for (List <Internal::CloseHandler>::Element * E =
            CloseHandlers.First; E; E = E->Next)
    {
        ToCall.Push (** E);
    }

    for (List <Internal::CloseHandler>::Element * E =
            ToCall.First; E; E = E->Next)
    {
        (** E).Proc (*Public, (** E).Tag);
    }

    Closing = false;
    
    -- UserCount;

    if (UserCount == 0 && !Public) /* see Stream destructor */
    {
        delete this;
        return;
    }
}

void Stream::Close ()
{
    internal->Close ();
}

void Stream::BeginQueue ()
{
    if (internal->BackQueue.Size)
    {
        /* Although we're going to start queueing any new data, whatever is
         * currently in the queue still needs to be written.  A Queued with
         * this flag set indicates where the stream should stop writing and
         * set Queueing to true.
         */

        internal->BackQueue.Push ();

        (** internal->BackQueue.Last).Flags |=
                    Internal::Queued::Flag_BeginQueue;

        internal->BackQueue.Push ();
    }
    else
    {
        internal->Queueing = true;
    }
}

size_t Stream::Queued ()
{
    size_t size = 0;

    for (List <Internal::Queued>::Element * E
                = internal->BackQueue.First; E; E = E->Next)
    {
        Internal::Queued &queued = ** E;

        size += queued.Buffer.Size - queued.Buffer.Offset;

        Stream::Internal * stream = queued.StreamPtr;

        if (!stream)
            continue;

        if (queued.StreamBytesLeft != -1)
        {
            size += (** E).StreamBytesLeft;
            continue;
        }

        size_t bytes = stream->Public->BytesLeft ();

        if (bytes == -1)
            return -1;

        size += bytes;
    }

    return size;
}

void Stream::EndQueue
        (int head_buffers, const char ** buffers, size_t * lengths)
{
    for (int i = 0; i < head_buffers; ++ i)
    {
        internal->Write (buffers [i], lengths [i],
                    Internal::Write_IgnoreQueue | Internal::Write_IgnoreBusy);
    }

    EndQueue ();
}

void Stream::EndQueue ()
{
    /* TODO : Look for a queued item w/ Flag_BeginQueue if Queueing is false? */

    LacewingAssert (internal->Queueing);

    internal->Queueing = false;

    internal->WriteQueued ();
}

bool Stream::Internal::IsTransparent ()
{
    LacewingAssert (Public);

    return (!ExpDataHandlers.Size) && (!Queueing) && Public->IsTransparent ();
}

bool Stream::IsTransparent ()
{
    return false;
}

void * Stream::Cast (void *)
{
    return 0;
}

void Stream::AddHandlerData (HandlerData Proc, void * Tag)
{   
    /* TODO : Prevent the same handler being registered twice? */

    Internal::DataHandler handler = { Proc, internal, Tag };
    internal->DataHandlers.Push (handler);

    internal->Graph->ClearExpanded ();
    internal->Graph->Expand ();
} 

void Stream::RemoveHandlerData (HandlerData Proc, void * Tag)
{   
    for (List <Internal::DataHandler>::Element * E =
            internal->DataHandlers.First; E; E = E->Next)
    {
        if ((** E).Proc == Proc)
        {
            internal->DataHandlers.Erase (E);
            break;
        }
    }

    internal->Graph->ClearExpanded ();
    internal->Graph->Expand ();
}

void Stream::AddHandlerClose (HandlerClose Proc, void * Tag)
{   
    Internal::CloseHandler handler = { Proc, Tag };
    internal->CloseHandlers.Push (handler);
} 

void Stream::RemoveHandlerClose (HandlerClose Proc, void * Tag)
{   
    for (List <Internal::CloseHandler>::Element * E =
            internal->CloseHandlers.First; E; E = E->Next)
    {
        if ((** E).Proc == Proc)
        {
            internal->CloseHandlers.Erase (E);
            break;
        }
    }
}



