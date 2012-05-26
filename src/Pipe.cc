
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

Pipe::Pipe ()
{
}

Pipe::Pipe (Lacewing::Pump &pump) : Stream (pump)
{
}

Pipe::~ Pipe ()
{
}

/* Because pipes are considered transparent, Pipe::Put will never actually be
 * called in most applications.  The data will be pushed directly to the next
 * stream in the graph.
 */

size_t Pipe::Put (const char * buffer, size_t size)
{
    if (!internal->DataHandlers.Size)
    {
        /* Skip Data() to avoid copying the buffer */

        internal->Push (buffer, size);
    }
    else
    {
        /* TODO : Limit on stack copy? */

        char * copy = (char *) alloca (size);
        memcpy (copy, buffer, size);

        Data (copy, size);
    }

    return size;
}

size_t Pipe::PutWritable (char * buffer, size_t size)
{
    Data (buffer, size);

    return size;
}

bool Pipe::IsTransparent ()
{
    return true;
}



