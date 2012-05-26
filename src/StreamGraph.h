
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

struct StreamGraph
{
    StreamGraph ();
    ~ StreamGraph ();


    /* Merges another graph into this one, and then deletes it. */

    void Swallow (StreamGraph *);


    /* Each StreamGraph actually stores two graphs - one public without the
     * filters, and an internally expanded version with the filters included.
     */

    struct Link
    {
        inline Link (Stream::Internal * _From, Stream::Internal * _FromExp, 
                        Stream::Internal * _To, Stream::Internal * _ToExp,
                           size_t _BytesLeft, int _LastPush, bool _DeleteStream)

            : From (_From), FromExp (_FromExp), To (_To), ToExp (_ToExp),
                BytesLeft (_BytesLeft), LastPush (_LastPush),
                    DeleteStream (_DeleteStream)
        {
        }

        Stream::Internal * To;
        Stream::Internal * ToExp;

        Stream::Internal * From;
        Stream::Internal * FromExp;

        size_t BytesLeft;
        int LastPush;

        bool DeleteStream;
    };

    List <Stream::Internal *> Roots;
    List <Stream::Internal *> RootsExpanded;

    void ClearExpanded ();

    void Expand ();
    void Read ();

    int LastPush;
    int LastRead;
    int LastExpand;

    /* Print the graph to DebugOut */

    void Print ();
};

