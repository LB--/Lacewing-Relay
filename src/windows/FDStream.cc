
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
    short RefCount;

    struct Overlapped
    {
        OVERLAPPED _Overlapped;

        char Type;

        const static char Type_Read          = 1;
        const static char Type_Write         = 2;
        const static char Type_TransmitFile  = 3;

        char Data [1];

    } * ReadOverlapped, * TransmitFileOverlapped;
    
    FDStream::Internal * TransmitFileFrom,
                       * TransmitFileTo;

    FDStream * Public;

    Lacewing::Pump &Pump;

    char Buffer [lwp_default_buffer_size];

    HANDLE FD;

    Pump::Watch * Watch;

    Internal (Lacewing::Pump &_Pump, FDStream * _Public)
        : Pump (_Pump), Public (_Public)
    {
        RefCount = 0;

        ReadingSize = 0;
        FD = INVALID_HANDLE_VALUE;

        Flags = Flag_Nagle;

        Size = -1;

        Watch = 0;
        
        PendingWrites = 0;

        Offset.QuadPart = 0;

        ReadOverlapped = new Overlapped ();
        TransmitFileOverlapped = new Overlapped ();

        TransmitFileFrom = 0;
        TransmitFileTo = 0;
    }

    ~ Internal ()
    {
        CloseFD ();
    
        /* TODO: May leak ReadOverlapped if no read is pending */
    }

    size_t Size;
    size_t ReadingSize;

    LARGE_INTEGER Offset;

    const static char
        Flag_ReadPending     = 1,
        Flag_Nagle           = 2,
        Flag_IsSocket        = 4,
        Flag_CloseASAP       = 8,  /* FD close pending on write? */
        Flag_AutoClose       = 16;

    char Flags;

    void IssueRead ()
    {
        if (Flags & Flag_ReadPending)
            return;

        memset (ReadOverlapped, 0, sizeof (*ReadOverlapped));
        ReadOverlapped->Type = Overlapped::Type_Read;

        ReadOverlapped->_Overlapped.Offset = Offset.LowPart;
        ReadOverlapped->_Overlapped.OffsetHigh = Offset.HighPart;

        size_t to_read = sizeof (Buffer);

        if (ReadingSize != -1 && to_read > ReadingSize)
            to_read = ReadingSize;

        if (ReadFile (FD, Buffer, to_read,
               0, (OVERLAPPED *) ReadOverlapped) == -1)
        {
            int error = GetLastError();

            if (error != ERROR_IO_PENDING)
            {
                Public->Stream::Close ();
                return;
            }
        }

        if (Size != -1)
            Offset.QuadPart += to_read;

        Flags |= Flag_ReadPending;
    }

    int PendingWrites;

    static void Completion
        (void * tag, OVERLAPPED * _overlapped,
                unsigned int bytes_transferred, int error)
    {
        FDStream::Internal * internal = (FDStream::Internal *) tag;
        Overlapped * overlapped = (Overlapped *) _overlapped;

        if (error == ERROR_OPERATION_ABORTED)
        {
            /* This operation was aborted - we have no guarantee that
             * internal is valid, so just delete the Overlapped and get out.
             */

            delete overlapped;
            return;
        }

        ++ internal->RefCount;

        switch (overlapped->Type)
        {
            case Overlapped::Type_Read:

                assert (overlapped == internal->ReadOverlapped);

                internal->Flags &= ~ Flag_ReadPending;

                if (error || !bytes_transferred)
                {
                    internal->Public->Stream::Close ();
                    break;
                }

                internal->Public->Data (internal->Buffer, bytes_transferred);

                /* Calling Data may result in stream destruction */

                if (!internal->Public)
                    break;

                internal->IssueRead ();
                break;

            case Overlapped::Type_Write:
                
                free (overlapped);

                if (internal->WriteCompleted ())
                {
                    delete internal->Public;
                    internal->Public = 0;
                }

                break;

            case Overlapped::Type_TransmitFile:
            {
                assert (overlapped == internal->TransmitFileOverlapped);

                internal->TransmitFileFrom->TransmitFileTo = 0;                            
                internal->TransmitFileFrom->WriteCompleted ();
                internal->TransmitFileFrom = 0;

                if (internal->WriteCompleted ())
                {
                    delete internal->Public;
                    internal->Public = 0;
                }

                break;
            }
        };

        if ((-- internal->RefCount) == 0 && !internal->Public)
            delete internal;
    }

    bool Close (bool immediate);

    void CloseFD ()
    {
        if (FD == INVALID_HANDLE_VALUE)
            return;

        if (TransmitFileFrom)
        {
            TransmitFileFrom->TransmitFileTo = 0;
            TransmitFileFrom->WriteCompleted ();
            TransmitFileFrom = 0;

            /* Not calling WriteCompleted for this stream because the FD
             * is closing anyway (and WriteCompleted may result in our
             * destruction, which would be annoying here.)
             */
        }

        if (TransmitFileTo)
        {
            TransmitFileTo->TransmitFileFrom = 0;
            TransmitFileTo->WriteCompleted ();
            TransmitFileTo = 0;
        }

        if (!CancelIo (FD))
        {
            assert (false);
        }

        PendingWrites = 0;

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

        /* If we're trying to close, was this the last write finishing? */

        if ( (Flags & Flag_CloseASAP) && PendingWrites == 0)
        {
            CloseFD ();
            
            Public->Close (true);
        }

        return false;
    }
};

FDStream::FDStream (Lacewing::Pump &Pump) : Stream (Pump)
{
    internal = new Internal (Pump, this);
}

FDStream::~ FDStream ()
{
    Close (true);

    if (internal->RefCount > 0)
        internal->Public = 0;
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
    return (internal->FD != INVALID_HANDLE_VALUE);
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

    Internal::Overlapped * overlapped =
        (Internal::Overlapped *) malloc (sizeof (Internal::Overlapped) + size);

    if (!overlapped)
        return size; 

    memset (overlapped, 0, sizeof (*overlapped));

    overlapped->Type = Internal::Overlapped::Type_Write;
    
    overlapped->_Overlapped.Offset = internal->Offset.LowPart;
    overlapped->_Overlapped.OffsetHigh = internal->Offset.HighPart;

    /* TODO : Find a way to avoid copying the data. */

    memcpy (overlapped->Data, buffer, size);

    /* TODO : Use WSASend if IsSocket == true?  (for better error messages.)
     * Same goes for ReadFile and WSARecv.
     */

    if (::WriteFile (internal->FD, overlapped->Data, size,
                        0, (OVERLAPPED *) overlapped) == -1)
    {
        int error = GetLastError ();

        if (error != ERROR_IO_PENDING)
        {
            /* TODO : send failed - anything to do here? */

            return size;
        }
    }

    if (internal->Size != -1)
        internal->Offset.QuadPart += size;

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
    {
        size = stream->BytesLeft ();

        if (size == -1)
            return -1;
    }

    if (size >= (((size_t) 1024) * 1024 * 1024 * 2))
        return -1; 

    /* TransmitFile can only send from a file to a socket */

    if (stream->internal->Flags & Internal::Flag_IsSocket)
        return -1;

    if (! (internal->Flags & Internal::Flag_IsSocket))
        return -1;

    if (internal->TransmitFileFrom || stream->internal->TransmitFileTo)
        return -1;  /* source or dest stream already performing a TransmitFile */

    Internal::Overlapped * overlapped = internal->TransmitFileOverlapped;
    memset (overlapped, 0, sizeof (*overlapped));

    overlapped->Type = Internal::Overlapped::Type_TransmitFile;

    /* Use the current offset from the source stream */

    ((OVERLAPPED *) overlapped)->Offset = stream->internal->Offset.LowPart;
    ((OVERLAPPED *) overlapped)->OffsetHigh = stream->internal->Offset.HighPart;

    /* TODO : Can we make use of the head buffers somehow?  Perhaps if data
     * is queued and preventing this TransmitFile from happening immediately,
     * the head buffers could be used to drain it.
     */

    if (!TransmitFile
            ((SOCKET) internal->FD, stream->internal->FD,
                (DWORD) size, 0, (OVERLAPPED *) overlapped, 0, 0))
    {
        int error = WSAGetLastError ();

        if (error != WSA_IO_PENDING)
            return -1;
    }

    /* OK, looks like the TransmitFile call succeeded. */

    internal->TransmitFileFrom = stream->internal;
    stream->internal->TransmitFileTo = internal;

    if (stream->internal->Size != -1)
        stream->internal->Offset.QuadPart += size;

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

    return internal->Size - ((size_t) internal->Offset.QuadPart);
}

/* Unlike with *nix, there's the possibility that data might actually be
 * pending in the FDStream, rather than just the Stream.  To account for this,
 * we override Close() to check PendingWrites first.
 */

bool FDStream::Close (bool immediate)
{
    return internal->Close (immediate);
}

bool FDStream::Internal::Close (bool immediate)
{
    if (immediate || PendingWrites == 0)
    {
        if (!Public->Stream::Close (immediate))
            return false;

        /* NOTE: Public may be deleted at this point */

        CloseFD ();
        return true;
    }
    else
    {
        Flags |= Flag_CloseASAP;
        return false;
    }
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



