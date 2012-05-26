
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

    void Data (char * buffer, size_t size);

    bool IsTransparent ();

    struct Filter
    {
        Stream::Internal * StreamPtr;
        bool Owned;
    };

    List <Filter> FiltersUpstream;
    List <Filter> FiltersDownstream;

    /* StreamGraph::Expand sets HeadUpstream to the head of the expanded
     * first filter, and fills ExpDataHandlers with the data handlers this
     * stream is responsible for calling.
     */

    Stream::Internal * HeadUpstream;

    List <DataHandler> ExpDataHandlers;

    struct Queued
    {
        /* For each Queued in the queue, the contents of Buffer will be written
         * first, and then the Stream.
         */
         
        HeapBuffer Buffer;

        Stream::Internal * StreamPtr;
        size_t StreamBytesLeft;

        inline Queued ()
        {
            this->StreamPtr = 0;
            Flags = 0;
        }

        char Flags;

        const static int Flag_BeginQueue = 1;
        const static int Flag_DeleteStream = 2;
    };

    List <Queued> FrontQueue; /* to be written before current source stream */
    List <Queued> BackQueue; /* to be written after current source stream */

    bool Queueing;

    void WriteQueued (List <Queued> &);
    void WriteQueued ();

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

    void Push (const char * buffer, size_t size);

    bool WriteDirect ();

    void Close ();
    bool Closing;

    int UserCount;
};


