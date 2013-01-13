
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

/* FDStream makes the assumption that this will fail for anything but a regular
 * file (i.e. something that is always considered read ready)
 */

static lw_i64 lwp_sendfile (int from, int dest, lw_i64 size)
{
   #if defined (__linux__)

      ssize_t sent = 0;
      
      if ((sent = sendfile (dest, from, 0, size)) == -1)
         return errno == EAGAIN ? 0 : -1;

      return sent;

   #elif defined (__FreeBSD__)

      off_t sent = 0;

      if (sendfile (from, dest, lseek (from, 0, SEEK_CUR), size, 0, &sent, 0) != 0)
         if (errno != EAGAIN)
            return -1;

      lseek (from, sent, SEEK_CUR);
      return sent;

   #elif defined (__APPLE__)

      off_t bytes = size;

      if (sendfile (from, dest, lseek (from, 0, SEEK_CUR), &bytes, 0, 0) != 0)
         if (errno != EAGAIN)
            return -1;

      lseek (from, bytes, SEEK_CUR);
      return bytes;

   #endif

   errno = EINVAL;
   return -1;
}

static void * FDStreamType = (void *) "FDStream";

void * FDStream::Cast (void * type)
{
    if (type == FDStreamType)
        return this;

    return Stream::Cast (type);
}

struct FDStream::Internal
{
    FDStream * Public;

    int RefCount;

    Lacewing::Pump &Pump;
    Pump::Watch * Watch;

    int FD;

    char Flags;

    const static char
        Flag_Nagle      = 1,
        Flag_IsSocket   = 2,
        Flag_AutoClose  = 4,
        Flag_Reading    = 8;

    Internal (Lacewing::Pump &_Pump, FDStream * _Public)
        : Pump (_Pump), Public (_Public)
    {
        RefCount = 0;

        Flags = Flag_Nagle;

        ReadingSize = 0;

        FD = -1;

        Watch = 0;
    }

    ~ Internal ()
    {
    }

    bool Close (bool immediate);

    size_t Size;

    static void WriteReady (void * tag)
    {
        ((Internal *) tag)->Public->Retry (Stream::Retry_Now);
    }

    static void ReadReady (void * tag)
    {
        ((Internal *) tag)->TryRead ();
    }

    size_t ReadingSize;

    void TryRead ()
    {
        if (FD == -1)
            return;

        if (Flags & Flag_Reading)
            return;

        Flags |= Flag_Reading;

        ++ RefCount;

        /* TODO : Use a buffer on the heap instead? */

        char buffer [lwp_default_buffer_size];

        while (ReadingSize == -1 || ReadingSize > 0)
        {
            size_t to_read = sizeof (buffer);

            if (ReadingSize != -1 && to_read > ReadingSize)
                to_read = ReadingSize;

            int bytes = read (FD, buffer, to_read);

            if (bytes == 0)
            {
                Public->Close (true);
                break;
            }

            if (bytes == -1)
            {
                if (errno == EAGAIN)
                    break;

                Public->Close (true);
                break;
            }

            if (ReadingSize != -1 && (ReadingSize -= bytes) < 0)
                ReadingSize = 0;

            Public->Data (buffer, bytes);

            /* Calling Data or Close may result in destruction of the Stream -
             * see FDStream destructor.
             */

            if (! (Flags & Flag_Reading))
                break;
        }

        Flags &= ~ Flag_Reading;

        if ((-- RefCount) == 0 && !Public)
            delete this;
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

void FDStream::SetFD (int FD, Pump::Watch * watch, bool auto_close)
{
    if (internal->Watch)
    {
        internal->Pump.Remove (internal->Watch);
        internal->Watch = 0;
    }

    if ( (internal->Flags & Internal::Flag_AutoClose) && internal->FD != -1)
        Close (true);

    internal->FD = FD;

    if (auto_close)
        internal->Flags |= Internal::Flag_AutoClose;
    else
        internal->Flags &= ~ Internal::Flag_AutoClose;

    if (FD == -1)
        return;

    fcntl (FD, F_SETFL, fcntl (FD, F_GETFL, 0) | O_NONBLOCK);

    #if HAVE_DECL_SO_NOSIGPIPE
    {   int b = 1;
        setsockopt (FD, SOL_SOCKET, SO_NOSIGPIPE, (char *) &b, sizeof (b));
    }
    #endif

    {   int b = (internal->Flags & Internal::Flag_Nagle) ? 0 : 1;
        setsockopt (FD, SOL_SOCKET, TCP_NODELAY, (char *) &b, sizeof (b));
    }

    internal->FD = FD;

    struct stat stat;
    fstat (FD, &stat);

    if (S_ISSOCK (stat.st_mode))
        internal->Flags |= Internal::Flag_IsSocket;
    else
        internal->Flags &= ~ Internal::Flag_IsSocket;

    if ((internal->Size = stat.st_size) > 0)
        return;

    /* Size is 0.  Is it really an empty file? */

    if (S_ISREG (stat.st_mode))
        return;

    internal->Size = -1;

    if (watch)
    {
        /* Given an existing Watch - change it to our callbacks */
        
        internal->Watch = watch;

        internal->Pump.UpdateCallbacks
            (watch, internal, Internal::ReadReady,
                    Internal::WriteReady, true);
    }
    else
    {
        internal->Watch = internal->Pump.Add
            (FD, internal, Internal::ReadReady, Internal::WriteReady);
    }
}

bool FDStream::Valid ()
{
    return internal->FD != -1;
}

size_t FDStream::Put (const char * buffer, size_t size)
{
    size_t written;

    #if HAVE_DECL_SO_NOSIGPIPE
        written = write (internal->FD, buffer, size);
    #else
        if (internal->Flags & Internal::Flag_IsSocket)
            written = send (internal->FD, buffer, size, MSG_NOSIGNAL);
        else
            written = write (internal->FD, buffer, size);
    #endif

    if (written == -1)
        return 0;

    return written;
}

size_t FDStream::Put (Stream &_stream, size_t size)
{
    FDStream * stream = (FDStream *) _stream.Cast (::FDStreamType);

    if (!stream)
        return -1;

    if (size == -1)
        size = stream->BytesLeft ();

    lw_i64 sent = lwp_sendfile (stream->internal->FD, internal->FD, size);

    lwp_trace ("lwp_sendfile sent " lwp_fmt_size " of " lwp_fmt_size,
                    ((size_t) sent), (size_t) size);

    return sent;
}

void FDStream::Read (size_t bytes)
{
    lwp_trace ("FDStream (FD %d) read " lwp_fmt_size, internal->FD, bytes);

    bool WasReading = internal->ReadingSize != 0;

    if (bytes == -1)
        internal->ReadingSize = BytesLeft ();
    else
        internal->ReadingSize += bytes;

    if (!WasReading)
        internal->TryRead ();
}

size_t FDStream::BytesLeft ()
{
    if (internal->FD == -1)
        return -1; /* not valid */

    if (internal->Size == -1)
        return -1;

    /* TODO : lseek64? */

    return internal->Size - lseek (internal->FD, 0, SEEK_CUR);
}

bool FDStream::Close (bool immediate)
{
    return internal->Close (immediate);
}

bool FDStream::Internal::Close (bool immediate)
{
    lwp_trace ("FDStream::Close for FD %d", FD);

    ++ RefCount;

    if (!Public->Stream::Close (immediate))
        return false;

    /* NOTE: Public may be deleted at this point */

    /* Remove the watch to make sure we don't get any more
     * callbacks from the pump.
     */

    if (Watch)
    {
        Pump.Remove (Watch);
        Watch = 0;
    }

    int FD = this->FD;

    this->FD = -1;

    if (FD != -1)
    {
        if (Watch)
        {
            Pump.Remove (Watch);
            Watch = 0;
        }

        if (Flags & Flag_AutoClose)
        {
            shutdown (FD, SHUT_RDWR);
            close (FD);
        }
    }

    if ((-- RefCount) == 0 && !Public)
        delete this;

    return true;
}

void FDStream::Cork ()
{
    #ifdef LacewingCork
        int enabled = 1;
        setsockopt (internal->FD, IPPROTO_TCP, LacewingCork, &enabled, sizeof (enabled));
    #endif
}

void FDStream::Uncork ()
{
    #ifdef LacewingCork
        int enabled = 0;
        setsockopt (internal->FD, IPPROTO_TCP, LacewingCork, &enabled, sizeof (enabled));
    #endif
}

void FDStream::Nagle (bool enabled)
{
    if (enabled)
        internal->Flags |= Internal::Flag_Nagle;
    else
        internal->Flags &= ~ Internal::Flag_Nagle;

    if (internal->FD != -1)
    {
        int b = enabled ? 0 : 1;
        setsockopt (internal->FD, SOL_SOCKET, TCP_NODELAY, (char *) &b, sizeof (b));
    }
}



