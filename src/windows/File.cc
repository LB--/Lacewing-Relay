
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
    DWORD dwDesiredAccess, dwShareMode, dwCreationDisposition;

    switch (*mode ++)
    {
        case 'r':

            dwDesiredAccess = GENERIC_READ;
            dwCreationDisposition = OPEN_EXISTING;
            dwShareMode = FILE_SHARE_READ;

            break;

        case 'w':

            dwDesiredAccess = GENERIC_WRITE;
            dwCreationDisposition = CREATE_ALWAYS;
            dwShareMode = 0;

            break;

        case 'a':

            dwDesiredAccess = GENERIC_WRITE;
            dwCreationDisposition = OPEN_ALWAYS;
            dwShareMode = 0;

            break;

        default:

            return false;
    };

    if (*mode == 'b')
        ++ mode;

    if (*mode == '+')
    {   
        dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
        ++ mode;
    }

    if (*mode == 'b')
        ++ mode;

    if (*mode == 'x')
    {
        if (dwDesiredAccess == GENERIC_READ)
            return false;

        dwCreationDisposition = CREATE_NEW;
    }

    /* TODO: should be converting to UTF-16 and using *W functions */

    HANDLE FD = CreateFileA (filename, dwDesiredAccess, dwShareMode,
                    0, dwCreationDisposition,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);

    if (FD == INVALID_HANDLE_VALUE)
        return false;

    SetFD (FD);

    return Valid ();
}

