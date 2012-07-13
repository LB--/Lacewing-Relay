
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

StreamGraph::StreamGraph ()
{
    LastPush = 0;
    LastRead = 0;
    LastExpand = 0;
}

StreamGraph::~ StreamGraph ()
{
    ClearExpanded ();
}

static void Swallow (StreamGraph * graph, Stream::Internal * stream)
{
    LacewingAssert (graph);
    LacewingAssert (stream->Graph);

    stream->Graph = graph;
    stream->LastExpand = graph->LastExpand;
   
    for (List <Stream::Internal::Filter>::Element * E
            = stream->FiltersUpstream.First; E; E = E->Next)
    {
        Swallow (graph, (** E).StreamPtr);
    }

    for (List <Stream::Internal::Filter>::Element * E
            = stream->FiltersDownstream.First; E; E = E->Next)
    {
        Swallow (graph, (** E).StreamPtr);
    }

    for (List <StreamGraph::Link *>::Element * E
            = stream->Next.First; E; E = E->Next)
    {
        Swallow (graph, (** E)->To);
    }
}

void StreamGraph::Swallow (StreamGraph * graph)
{
    LacewingAssert (graph != this);

    for (List <Stream::Internal *>::Element * E
                = graph->Roots.First; E; E = E->Next)
    {
        Stream::Internal * stream = (** E);

        Roots.Push (stream);

        ::Swallow (this, stream);
    }

    delete graph;
}

static void Expand (StreamGraph * graph, Stream::Internal * stream,
                        Stream::Internal *& first, Stream::Internal *& last)
{
    first = last = 0;


    /* Upstream filters come first */

    for (List <Stream::Internal::Filter>::Element * E
            = stream->FiltersUpstream.First; E; E = E->Next)
    {
        Stream::Internal * stream = (** E).StreamPtr, * expanded, * next;
       
        stream->LastExpand = graph->LastExpand;

        Expand (graph, stream, expanded, next);

        if (!first)
            first = expanded;
        else
        {
            StreamGraph::Link * link = new (std::nothrow) StreamGraph::Link
                (0, last, 0, expanded, -1, stream->Graph->LastPush, false);

            last->NextExpanded.Push (link);
            expanded->PrevExpanded.Push (link);
        }

        last = next;
    }

    stream->HeadUpstream = first;


    /* Now the stream itself */

    stream->LastExpand = graph->LastExpand;

    if (last)
    {
        StreamGraph::Link * link
            = new (std::nothrow) StreamGraph::Link
                (0, last, 0, stream, -1, stream->Graph->LastPush, false);

        last->NextExpanded.Push (link);
        stream->PrevExpanded.Push (link);

        last = stream;
    }
    else
    {
        first = last = stream;
    }


    /* And downstream filters afterwards */

    for (List <Stream::Internal::Filter>::Element * E
            = stream->FiltersDownstream.First; E; E = E->Next)
    {
        Stream::Internal * stream = (** E).StreamPtr, * expanded, * next;
       
        stream->LastExpand = graph->LastExpand;

        Expand (graph, stream, expanded, next);

        if (last)
        {
            StreamGraph::Link * link
                = new (std::nothrow) StreamGraph::Link
                    (0, last, 0, expanded, -1, stream->Graph->LastPush, false);

            last->NextExpanded.Push (link);
            expanded->PrevExpanded.Push (link);
        }
        
        last = next;
    }


    /* The last downstream filter is in charge of calling any data handlers */

    for (List <Stream::Internal::DataHandler>::Element * E
            = stream->DataHandlers.First; E; E = E->Next)
    {
        last->ExpDataHandlers.Push (** E);
    }
}

static void Expand (StreamGraph * graph, Stream::Internal * last,
                        StreamGraph::Link * last_link, StreamGraph::Link * link,
                            Stream::Internal * stream)
{
    Stream::Internal * expanded_first, * expanded_last;

    Expand (graph, stream, expanded_first, expanded_last);

    if (last)
    {
        link->ToExp = expanded_first;

        expanded_first->PrevExpanded.Push (link);

        link->FromExp = last;
        last->NextExpanded.Push (link);
    }
    else
    {
        graph->RootsExpanded.Push (expanded_first);
    }

    last = expanded_last;

    for (List <StreamGraph::Link *>::Element * E = stream->Next.First;
            E; E = E->Next)
    {
        StreamGraph::Link * link = (** E);

        Expand (graph, last, last_link, link, link->To);
    }
}

void StreamGraph::Expand ()
{
    ClearExpanded ();

    for (List <Stream::Internal *>::Element * E
            = Roots.First; E; E = E->Next)
    {
        Stream::Internal * stream = (** E);

        if (stream->LastExpand == LastExpand)
            continue;
                
        ::Expand (this, 0, 0, 0, stream);
    }

    for (List <Stream::Internal *>::Element * E
            = RootsExpanded.First; E; )
    {
        /* If a root appears elsewhere in the expanded graph, it
         * doesn't need to be a root.
         */

        if ((** E)->PrevExpanded.Size)
            E = RootsExpanded.Erase (E);
        else
            E = E->Next;
    }

    /* #ifdef LacewingDebug
        Print ();
    #endif */
}

static bool FindNextDirect
    (Stream::Internal * stream, Stream::Internal *& next_direct, size_t &bytes)
{
    for (List <StreamGraph::Link *>::Element * E
            = stream->NextExpanded.First; E; E = E->Next)
    {
        StreamGraph::Link * link = (** E);

        if (link->BytesLeft != -1 && link->BytesLeft < bytes)
            bytes = link->BytesLeft;

        Stream::Internal * next = link->ToExp;

        if (next->IsTransparent ())
        {
            if (!FindNextDirect (next, next_direct, bytes))
                return false;
        }

        if (next)
        {
            if (next_direct)
                return false;

            next_direct = next;
        }
    }

    return true;
}

static void Read (StreamGraph * graph, int this_read,
                    Stream::Internal * stream, size_t bytes)
{
    /* Note that we're checking Next instead of NextExpanded here.  The
     * presence of a filter doesn't force a read.
     */

    if ( (!stream->Next.Size) && (!stream->ExpDataHandlers.Size) )
        return;

    if (bytes == -1)
    {
        /* Use the biggest BytesLeft from all the links */

        for (List <StreamGraph::Link *>::Element * E
                = stream->Next.First; E; E = E->Next)
        {
            StreamGraph::Link * link = (** E);

            if (link->BytesLeft == -1)
            {
                bytes = -1;
                break;
            }
            
            if (link->BytesLeft > bytes)
                bytes = link->BytesLeft;
        }
    }

    bool wrote_direct = false;

    do
    {
        if (stream->ExpDataHandlers.Size)
            break;

        Stream::Internal * next = 0;

        size_t direct_bytes = bytes;

        FindNextDirect (stream, next, direct_bytes);

        if (next)
        {
            /* Only one non-transparent stream follows.  It may be possible to
             * shift the data directly (without having to read it first).
             *
             * Note that we don't have to check next's queue is empty, because
             * we're already being written (the queue is behind us).
             */

            next->PrevDirect = stream;
            next->DirectBytesLeft = direct_bytes;

            if (next->WriteDirect ())
            {
                wrote_direct = true;
                break;
            }
        }
        else
        {
            stream->Public->Read (bytes);
        }

    } while (0);

    if (wrote_direct)
        return;

    /* Calling Push or Read may well run user code, or expire a link.  If this
     * causes the graph to re-expand, Read() will be called again and this one
     * must abort.
     *
     * TODO: What if this graph gets swallowed?
     */

    if (this_read != graph->LastRead)
        return;

    for (List <StreamGraph::Link *>::Element * E
            = stream->NextExpanded.First; E; E = E->Next)
    {
        StreamGraph::Link * link = (** E);

        Read (graph, this_read, link->ToExp, link->BytesLeft);

        if (this_read != graph->LastRead)
            return;
    }
}

void StreamGraph::Read ()
{
    int this_read = LastRead ++;

    for (List <Stream::Internal *>::Element * E
            = RootsExpanded.First; E; E = E->Next)
    {
        ::Read (this, this_read, (** E), -1);

        if (LastRead != this_read)
            break;
    }
}

static void ClearExpanded (Stream::Internal * stream)
{
    for (List <StreamGraph::Link *>::Element * E
            = stream->NextExpanded.First; E; E = E->Next)
    {
        StreamGraph::Link * link = (** E);

        if (link->ToExp)
            ClearExpanded (link->ToExp);

        /* If this link only exists in the expanded graph, delete it */

        if (!link->To)
            delete link;
        else
        {
            link->ToExp = link->FromExp = 0;
        }
    }

    stream->PrevExpanded.Clear ();
    stream->NextExpanded.Clear ();

    stream->PrevDirect = 0;

    stream->ExpDataHandlers.Clear ();
}

void StreamGraph::ClearExpanded ()
{
    ++ LastExpand;

    for (List <Stream::Internal *>::Element * E
            = RootsExpanded.First; E; E = E->Next)
    {
        ::ClearExpanded (** E);
    }

    RootsExpanded.Clear ();
}


/* debugging crap */

static void Print (StreamGraph * graph, Stream::Internal * stream, int depth)
{
    LacewingAssert (stream->Graph == graph);

    fprintf (stderr, "stream @ %p (" lw_fmt_size " bytes, %d handlers)\n",
               stream, stream->Public->BytesLeft (), stream->DataHandlers.Size);

    for (List <StreamGraph::Link *>::Element * E
            = stream->Next.First; E; E = E->Next)
    {
        for (int i = 0; i < depth; ++ i)
            fprintf (stderr, "  ");

        fprintf (stderr, "link %p ==> ", (** E));

        Print (graph, (** E)->To, depth + 1);
    }
}

static void PrintExpanded (StreamGraph * graph, Stream::Internal * stream, int depth)
{
    LacewingAssert (stream->Graph == graph);

    fprintf (stderr, "stream @ %p (" lw_fmt_size " bytes, %d handlers)\n",
                stream, stream->Public->BytesLeft (), stream->ExpDataHandlers.Size);

    for (List <StreamGraph::Link *>::Element * E
            = stream->NextExpanded.First; E; E = E->Next)
    {
        for (int i = 0; i < depth; ++ i)
            fprintf (stderr, "  ");

        fprintf (stderr, "link %p ==> ", (** E));

        LacewingAssert ((** E)->FromExp == stream);

        PrintExpanded (graph, (** E)->ToExp, depth + 1);
    }
}

void StreamGraph::Print ()
{
    fprintf (stderr, "\n--- Graph %p (%d) ---\n\n", this, Roots.Size);

    for (List <Stream::Internal *>::Element * E
            = Roots.First; E; E = E->Next)
    {
        ::Print (this, ** E, 1);
    }

    fprintf (stderr, "\n--- Graph %p expanded (%d) ---\n\n", this, RootsExpanded.Size);

    for (List <Stream::Internal *>::Element * E
            = RootsExpanded.First; E; E = E->Next)
    {
        ::PrintExpanded (this, ** E, 1);
    }

    fprintf (stderr, "\n");
}

