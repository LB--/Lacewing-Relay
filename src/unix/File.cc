
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

#include "../Common.h"

static int GetFlags (const char * mode);

File::File (Lacewing::Pump &Pump) : FDStream (Pump)
{
}

File::File (Lacewing::Pump &Pump, const char * filename, const char * mode)
                : FDStream (Pump)
{
    Open (filename, mode);
}

File::~ File ()
{
}

bool File::Open (const char * filename, const char * mode)
{
    DebugOut ("%p : File::Open \"%s\", \"%s\"",
                    ((Stream *) (FDStream *) this)->internal, filename, mode);

    int flags = GetFlags (mode);

    if (flags == -1)
        return false;

    int FD = open (filename, flags);

    if (FD == -1)
        return false;

    SetFD (FD);

    return Valid ();
}

int GetFlags (const char * mode)
{
    /* Based on what FreeBSD does to convert the mode string for fopen(3) */

    int flags;

    switch (*mode ++)
    {
        case 'r':
            flags = O_RDONLY;
            break;

        case 'w':
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            break;

        case 'a':
            flags = O_WRONLY | O_CREAT | O_APPEND;
            break;

        default:
            return -1;
    };

    if (*mode == 'b')
        ++ mode;

    if (*mode == '+')
    {   
        flags |= O_RDWR;
        ++ mode;
    }

    if (*mode == 'b')
        ++ mode;

    if (*mode == 'x')
    {
        if (flags & O_RDONLY)
            return -1;

        flags |= O_EXCL;
    }

    return flags;
}


