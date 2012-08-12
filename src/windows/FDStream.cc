
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

#include "../lw_common.h"

static void * FDStreamType = (void *) "FDStream";

void * FDStream::Cast (void * type)
{
    if (type == FDStreamType)
        return this;

    return Stream::Cast (type);
}

struct FDStream::Internal
{
    struct Overlapped
    {
        OVERLAPPED _Overlapped;

        char Type;

        const static char Type_Read          = 1;
        const static char Type_Write         = 2;
        const static char Type_TransmitFile  = 3;
    };

    struct TransmitFileOverlapped
    {
        Overlapped _Overlapped;

        FDStream::Internal * Source;
    };

    FDStream &Public;

    Lacewing::Pump &Pump;

    char Buffer [lwp_default_buffer_size];

    HANDLE FD;

    Pump::Watch * Watch;

    Internal (Lacewing::Pump &_Pump, FDStream &_Public)
        : Pump (_Pump), Public (_Public)
    {
        ReadingSize = 0;
        FD = INVALID_HANDLE_VALUE;

        Flags = Flag_Nagle;

        Size = -1;

        Watch = 0;
        
        PendingWrites = 0;
    }

    ~ Internal ()
    {
        CloseFD ();
    }

    size_t Size;

    size_t ReadingSize;

    const static char Flag_ReceivePending  = 1;
    const static char Flag_CompletionProc  = 2;
    const static char Flag_Nagle           = 4;
    const static char Flag_IsSocket        = 8;
    const static char Flag_Dead            = 16; /* public destroyed? */
    const static char Flag_Closed          = 32; /* FD close pending on write? */
    const static char Flag_AutoClose       = 64;

    char Flags;

    Overlapped ReceiveOverlapped;

    void IssueRead ()
    {
        if (Flags & Flag_ReceivePending)
            return;

        memset (&ReceiveOverlapped, 0, sizeof (ReceiveOverlapped));
        ReceiveOverlapped.Type = Overlapped::Type_Read;

        size_t to_read = sizeof (Buffer);

        if (ReadingSize != -1 && to_read > ReadingSize)
            to_read = ReadingSize;

        DWORD flags = 0;

        if (ReadFile (FD, Buffer, to_read, 0,
                        (OVERLAPPED *) &ReceiveOverlapped) == -1)
        {
            int error = GetLastError();

            if (error != ERROR_IO_PENDING)
            {
                Public.Stream::Close ();
                return;
            }
        }

        Flags |= Flag_ReceivePending;
    }

    int PendingWrites;

    static void Completion
        (void * tag, OVERLAPPED * _overlapped,
                unsigned int bytes_transferred, int error)
    {
        FDStream::Internal * internal = (FDStream::Internal *) tag;
        Overlapped * overlapped = (Overlapped *) _overlapped;

        assert (! (internal->Flags & Flag_CompletionProc));

        internal->Flags |= Flag_CompletionProc;

        switch (overlapped->Type)
        {
            case Overlapped::Type_Read:

                internal->Flags &= ~ Flag_ReceivePending;

                if (error || !bytes_transferred)
                {
                    internal->Public.Stream::Close ();
                    break;
                }

                internal->Public.Data (internal->Buffer, bytes_transferred);

                /* Calling Data may result in stream destruction */

                if (internal->Flags & Flag_Dead)
                {
                    delete internal;
                    return;
                }

                internal->IssueRead ();

                break;

            case Overlapped::Type_Write:
                
                delete overlapped;

                if (internal->WriteCompleted ())
                {
                    delete internal;
                    return;
                }

                break;

            case Overlapped::Type_TransmitFile:
            {
                ((TransmitFileOverlapped *) overlapped)
                    ->Source->WriteCompleted ();

                delete ((TransmitFileOverlapped *) overlapped);

                if (internal->WriteCompleted ())
                {
                    delete internal;
                    return;
                }

                break;
            }
        };

        internal->Flags &= ~ Flag_CompletionProc;

        if (internal->Flags & Flag_Dead
                && internal->PendingWrites == 0)
        {
            delete internal;
        }
    }

    void CloseFD ()
    {
        if (Flags & Flag_AutoClose)
        {
            if (Flags & Flag_IsSocket)
                closesocket ((SOCKET) FD);
            else
                CloseHandle (FD);
        }

        FD = INVALID_HANDLE_VALUE;
    }

    bool WriteCompleted ()
    {
        -- PendingWrites;

        /* Was this the last write finishing? */

        if ( (Flags & (Flag_Closed | Flag_Dead)) && PendingWrites == 0)
        {
            CloseFD ();
            
            if (Flags & Flag_Dead)
                return true;
        }

        return false;
    }
};

FDStream::FDStream (Lacewing::Pump &Pump) : Stream (Pump)
{
    internal = new Internal (Pump, *this);
}

FDStream::~ FDStream ()
{
    Close ();

    if (internal->Flags & Internal::Flag_CompletionProc
            || internal->PendingWrites > 0)
    {
        internal->Flags |= Internal::Flag_Dead;
    }
    else
        delete internal;
}

void FDStream::SetFD (HANDLE FD, Pump::Watch * watch, bool auto_close)
{
    if (internal->Watch)
        internal->Pump.Remove (internal->Watch);

    internal->FD = FD;

    if (FD == INVALID_HANDLE_VALUE)
        return;

    WSAPROTOCOL_INFO info;

    internal->Flags |= Internal::Flag_IsSocket;
    
    if (auto_close)
        internal->Flags |= Internal::Flag_AutoClose;

    if (WSADuplicateSocket
            ((SOCKET) FD, GetCurrentProcessId (), &info) == SOCKET_ERROR)
    {
        int error = WSAGetLastError ();

        if (error == WSAENOTSOCK || error == WSAEINVAL)
        { 
            internal->Flags &= ~ Internal::Flag_IsSocket;
        }
    }
 
    if (internal->Flags & Internal::Flag_IsSocket)
    {
        internal->Size = -1;

        {   int b = (internal->Flags & Internal::Flag_Nagle) ? 0 : 1;

            setsockopt ((SOCKET) FD, SOL_SOCKET,
                            TCP_NODELAY, (char *) &b, sizeof (b));
        }
    }
    else
    {
        LARGE_INTEGER size;

        if (!compat_GetFileSizeEx () (internal->FD, &size))
            return;

        internal->Size = (size_t) size.QuadPart;
        
        assert (internal->Size != -1);
    }

    if (watch)
    {
        internal->Watch = watch;
        internal->Pump.UpdateCallbacks (watch, internal, Internal::Completion);
    }
    else
    {
        internal->Watch = internal->Pump.Add
                (FD, internal, Internal::Completion);
    }
}

bool FDStream::Valid ()
{
    return (! (internal->Flags & (Internal::Flag_Closed | Internal::Flag_Dead)))
                && internal->FD != INVALID_HANDLE_VALUE;
}

/* Note that this always swallows all of the data (unlike on *nix where it
 * might only be able to use some of it.)  Because Windows FDStream never
 * calls WriteReady(), it's important that nothing gets buffered in the
 * Stream.
 */

size_t FDStream::Put (const char * buffer, size_t size)
{
    if (!size)
        return size; /* nothing to do */

    /* TODO : Pre-allocate a bunch of these and reuse them? */

    Internal::Overlapped * overlapped
        = new (std::nothrow) Internal::Overlapped;

    if (!overlapped)
        return size; 

    memset (overlapped, 0, sizeof (*overlapped));

    overlapped->Type = Internal::Overlapped::Type_Write;

    /* TODO : Assuming WriteFile is going to copy the buffer immediately.  I
     * don't know if that's guaranteed (but it seems to be the case).
     */

    /* TODO : Use WSASend if IsSocket == true?  (for better error messages.)
     * Same goes for ReadFile and WSARecv.
     */

    if (::WriteFile (internal->FD, buffer, size,
                        0, (OVERLAPPED *) overlapped) == -1)
    {
        int error = GetLastError ();

        if (error != ERROR_IO_PENDING)
        {
            /* TODO : send failed - anything to do here? */

            return size;
        }
    }

    ++ internal->PendingWrites;

    return size;
}

size_t FDStream::Put (Stream &_stream, size_t size)
{
    FDStream * stream = (FDStream *) _stream.Cast (::FDStreamType);

    if (!stream)
        return -1;

    if (!stream->Valid ())
        return -1;

    if (size == -1)
        size = stream->BytesLeft ();

    if (size >= (((size_t) 1024) * 1024 * 1024 * 2))
        return -1; 

    /* TransmitFile can only send from a file to a socket */

    if (stream->internal->Flags & Internal::Flag_IsSocket)
        return -1;

    if (! (internal->Flags & Internal::Flag_IsSocket))
        return -1;
    
    Internal::TransmitFileOverlapped * overlapped
        = new (std::nothrow) Internal::TransmitFileOverlapped;

    if (!overlapped)
        return -1;

    memset (overlapped, 0, sizeof (*overlapped));

    overlapped->_Overlapped.Type = Internal::Overlapped::Type_TransmitFile;
    overlapped->Source = stream->internal;

    /* TODO : Can we make use of the head buffers somehow? */

    if (!TransmitFile
            ((SOCKET) internal->FD, stream->internal->FD,
                (DWORD) size, 0, (OVERLAPPED *) overlapped, 0, 0))
    {
        int error = WSAGetLastError ();

        if (error != WSA_IO_PENDING)
            return -1;
    }

    ++ internal->PendingWrites;
    ++ stream->internal->PendingWrites;

    return size;
}

void FDStream::Read (size_t bytes)
{
    bool WasReading = internal->ReadingSize != 0;

    if (bytes == -1)
        internal->ReadingSize = BytesLeft ();
    else
        internal->ReadingSize += bytes;

    if (!WasReading)
        internal->IssueRead ();
}

size_t FDStream::BytesLeft ()
{
    if (! Valid ())
        return -1;

    if (internal->Size == -1)
        return -1;

    LARGE_INTEGER offset;

    LARGE_INTEGER zero;
    memset (&zero, 0, sizeof (zero));

    SetFilePointerEx (internal->FD, zero, &offset, FILE_CURRENT);
    
    return internal->Size - ((size_t) offset.QuadPart);
}

void FDStream::Close ()
{
    if (internal->Flags & (Internal::Flag_Closed | Internal::Flag_Dead))
        return;

    if (internal->PendingWrites > 0)
    {
        /* Leave the FD around while the remaining writes finish */
    }
    else
        internal->CloseFD ();

    Stream::Close ();
}

void FDStream::Nagle (bool enabled)
{
    if (enabled)
        internal->Flags |= Internal::Flag_Nagle;
    else
        internal->Flags &= ~ Internal::Flag_Nagle;

    if (Valid ())
    {
        int b = enabled ? 0 : 1;

        setsockopt ((SOCKET) internal->FD, SOL_SOCKET,
                        TCP_NODELAY, (char *) &b, sizeof (b));
    }
}

/* TODO : Can we do anything here on Windows? */

void FDStream::Cork ()
{
}

void FDStream::Uncork ()
{
}



