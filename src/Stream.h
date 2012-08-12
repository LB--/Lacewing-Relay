
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

#pragma once

#include "HeapBuffer.h"
#include "StreamGraph.h"

struct Stream::Internal
{
    Stream * Public;

    Lacewing::Pump * Pump;
    
    Internal (Stream *);
    ~ Internal ();

    struct DataHandler
    {
        Stream::HandlerData Proc;

        Stream::Internal * StreamPtr;
        void * Tag;
    };
    
    List <DataHandler> DataHandlers;

    struct CloseHandler
    {
        Stream::HandlerClose Proc;
        void * Tag;
    };

    List <CloseHandler> CloseHandlers;


    /* Calls any data handlers, then pushes the data forward */

    void Data (const char * buffer, size_t size);


    /* Returns true if this stream should be considered transparent, based on
     * whether the public IsTransparent returns true, no data handlers are
     * registered, and the queue is empty.
     */

    bool IsTransparent ();

    
    /* Pushes data forward to any streams next in the graph.  buffer may be
     * 0, in which case this is used to indicate the success of a direct write
     */

    void Push (const char * buffer, size_t size);


    struct Filter
    {
        Stream::Internal * StreamPtr;
        Stream::Internal * FilterPtr;

        bool DeleteWithStream;
        bool CloseTogether;

        /* Having a pre-allocated link here saves StreamGraph from having to
         * allocate one for each filter.
         */

        StreamGraph::Link Link;
    };


    /* Filters affecting this stream (StreamPtr == this).  The Filter should be
     * freed when removed from these lists.
     */

    List <Filter *> FiltersUpstream;
    List <Filter *> FiltersDownstream;


    /* Streams we are a filter for (FilterPtr == this) */

    List <Filter *> Filtering;


    /* StreamGraph::Expand sets HeadUpstream to the head of the expanded
     * first filter, and fills ExpDataHandlers with the data handlers this
     * stream is responsible for calling.
     */

    Stream::Internal * HeadUpstream;

    List <DataHandler> ExpDataHandlers;

    struct Queued
    {
        char Type;
         
        const static char Type_Data = 1;
        const static char Type_Stream = 2;
        const static char Type_BeginMarker = 3;

        HeapBuffer Buffer;

        Stream::Internal * StreamPtr;
        size_t StreamBytesLeft;

        inline Queued ()
        {
            this->StreamPtr = 0;
            Flags = 0;
        }

        char Flags;
        const static int Flag_DeleteStream = 1;
    };

    List <Queued> FrontQueue; /* to be written before current source stream */
    List <Queued> BackQueue; /* to be written after current source stream */

    bool Queueing;

    void WriteQueued (List <Queued> &);
    void WriteQueued ();

    int Retry;

    const static int Write_IgnoreFilters = 1;
    const static int Write_IgnoreBusy = 2;
    const static int Write_IgnoreQueue = 4;
    const static int Write_Partial = 8;
    const static int Write_DeleteStream = 16;

    void Write (Stream::Internal * stream, size_t size, int flags);
    size_t Write (const char * buffer, size_t size, int flags);

    StreamGraph * Graph;

    List <StreamGraph::Link *> Prev, Next;
    List <StreamGraph::Link *> PrevExpanded, NextExpanded;

    int LastExpand;

    Stream::Internal * PrevDirect;
    size_t DirectBytesLeft;

    /* Attempts to write data from PrevDirect, returning false on failure. If
     * successful, DirectBytesLeft will be adjusted.
     */

    bool WriteDirect ();

    void Close ();
    bool Closing;


    /* If UserCount is > 0 and the Stream is deleted, the internal structure
     * will not be freed.  Anything decrementing UserCount should be prepared
     * to delete the Stream if necessary.
     */

    int UserCount;
};


