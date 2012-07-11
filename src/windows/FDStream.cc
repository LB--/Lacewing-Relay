
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

static void * FDStreamType = (void *) "FDStream";

void * FDStream::Type ()
{
    return FDStreamType;
}

struct FDStreamOverlapped
{
    OVERLAPPED Overlapped;

    char Type;

    const static char Type_Read          = 1;
    const static char Type_Write         = 2;
    const static char Type_TransmitFile  = 3;
};

struct FDStream::Internal
{
    FDStream &Public;

    Lacewing::Pump &Pump;

    char Buffer [DefaultBufferSize];

    HANDLE FD;

    Pump::Watch * Watch;

    Internal (Lacewing::Pump &_Pump, FDStream &_Public)
        : Pump (_Pump), Public (_Public)
    {
        Nagle = true;

        ReadingSize = 0;
        FD = INVALID_HANDLE_VALUE;

        ReceivePending = false;
        CompletionProc = false;

        Size = -1;

        Dead = false;

        Watch = 0;
    }

    ~ Internal ()
    {
        Public.Close ();
    }

    size_t Size;

    size_t ReadingSize;

    /* TODO : Use flags instead? */

    bool ReceivePending;
    bool CompletionProc;
    bool Dead;
    bool Nagle;
    bool IsSocket;

    FDStreamOverlapped ReceiveOverlapped;

    void IssueRead ()
    {
        if (ReceivePending)
            return;

        memset (&ReceiveOverlapped, 0, sizeof (ReceiveOverlapped));
        ReceiveOverlapped.Type = FDStreamOverlapped::Type_Read;

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

        ReceivePending = true;
    }

    static void Completion
        (void * tag, OVERLAPPED * _overlapped,
                unsigned int bytes_transferred, int error)
    {
        FDStream::Internal * internal = (FDStream::Internal *) tag;
        FDStreamOverlapped * overlapped = (FDStreamOverlapped *) _overlapped;

        LacewingAssert (!internal->CompletionProc);

        internal->CompletionProc = true;

        switch (overlapped->Type)
        {
            case FDStreamOverlapped::Type_Read:

                internal->ReceivePending = false;

                if (error || !bytes_transferred)
                {
                    internal->Public.Stream::Close ();
                    break;
                }

                internal->Public.Data (internal->Buffer, bytes_transferred);

                /* Calling Data may result in stream destruction */

                if (internal->Dead)
                {
                    delete internal;
                    return;
                }

                internal->IssueRead ();

                break;

            case FDStreamOverlapped::Type_Write:

                delete overlapped;
                break;

            case FDStreamOverlapped::Type_TransmitFile:

                delete overlapped;
                break;
        };

        internal->CompletionProc = false;
    }
};

FDStream::FDStream (Lacewing::Pump &Pump) : Stream (Pump)
{
    internal = new Internal (Pump, *this);
}

FDStream::~ FDStream ()
{
    if (internal->CompletionProc)
        internal->Dead = true;
    else
        delete internal;
}

void FDStream::SetFD (HANDLE FD, Pump::Watch * watch)
{
    if (internal->Watch)
        internal->Pump.Remove (internal->Watch);

    internal->FD = FD;

    if (FD == INVALID_HANDLE_VALUE)
        return;

    WSAPROTOCOL_INFO info;

    internal->IsSocket = true;

    if (WSADuplicateSocket
            ((SOCKET) FD, GetCurrentProcessId (), &info) == SOCKET_ERROR)
    {
        int error = WSAGetLastError ();

        if (error == WSAENOTSOCK || error == WSAEINVAL)
        { 
            internal->IsSocket = false;
        }
    }
 
    if (internal->IsSocket)
    {
        internal->Size = -1;

        {   int b = internal->Nagle ? 0 : 1;

            setsockopt ((SOCKET) FD, SOL_SOCKET,
                            TCP_NODELAY, (char *) &b, sizeof (b));
        }
    }
    else
    {
        LARGE_INTEGER size;

        if (!Compat::GetFileSizeEx () (internal->FD, &size))
            return;

        internal->Size = size.QuadPart;

        LacewingAssert (internal->Size != -1);
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
    return internal->FD != INVALID_HANDLE_VALUE;
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

    FDStreamOverlapped * overlapped
        = new (std::nothrow) FDStreamOverlapped ();

    if (!overlapped)
        return size; 

    overlapped->Type = FDStreamOverlapped::Type_Write;

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

    return size;
}

size_t FDStream::Put (Stream &stream, size_t size)
{
    if (size == -1)
        size = stream.BytesLeft ();

    if (stream.Type () != ::FDStreamType)
        return -1; 

    if (size >= (((size_t) 1024) * 1024 * 1024 * 2))
        return -1; 

    /* TransmitFile can only send from a file to a socket */

    if (((FDStream *) &stream)->internal->IsSocket)
        return -1;

    if (!internal->IsSocket)
        return -1;
    
    FDStreamOverlapped * overlapped
        = new (std::nothrow) FDStreamOverlapped ();

    if (!overlapped)
        return -1;

    overlapped->Type = FDStreamOverlapped::Type_TransmitFile;

    /* TODO : Can we make use of the head buffers somehow? */

    if (!TransmitFile
            ((SOCKET) internal->FD, ((FDStream *) &stream)->internal->FD,
                (DWORD) size, 0, (OVERLAPPED *) overlapped, 0, 0))
    {
        int error = WSAGetLastError ();

        if (error != WSA_IO_PENDING)
            return -1;
    }

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
    if (internal->FD == INVALID_HANDLE_VALUE)
        return -1;

    if (internal->Size == -1)
        return -1;

    LARGE_INTEGER offset;

    LARGE_INTEGER zero;
    memset (&zero, 0, sizeof (zero));

    SetFilePointerEx (internal->FD, zero, &offset, FILE_CURRENT);
    
    return internal->Size - offset.QuadPart;
}

void FDStream::Close ()
{
    if (internal->IsSocket)
        closesocket ((SOCKET) internal->FD);
    else
        CloseHandle (internal->FD);

    Stream::Close ();
}

void FDStream::Nagle (bool enabled)
{
    internal->Nagle = enabled;

    if (internal->FD != INVALID_HANDLE_VALUE)
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



