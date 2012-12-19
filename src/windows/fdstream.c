
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2012 James McLaughlin et al.  All rights reserved.
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

#include "../common.h"

enum overlapped_type
{
    overlapped_type_read,
    overlapped_type_write,
    overlapped_type_transmitfile
};

struct overlapped
{
    OVERLAPPED overlapped;

    overlapped_type type;

    char data [];

} * read_overlapped, * transmitfile_overlapped;

const static char
    lwp_fdstream_flag_read_pending     = 1,
    lwp_fdstream_flag_nagle            = 2,
    lwp_fdstream_flag_is_socket        = 4,
    lwp_fdstream_flag_close_asap       = 8,  /* FD close pending on write? */
    lwp_fdstream_flag_auto_close       = 16,
    lwp_fdstream_flag_dead             = 32;

struct lw_fdstream
{
   struct lw_stream stream;

   short ref_count;

   overlapped * read_overlapped;
   overlapped * transmitfile_overlapped;

   lw_fdstream transmit_file_from,
               transmit_file_to;

   char buffer [lwp_default_buffer_size];

   HANDLE fd;

   lw_pump_watch watch;

   size_t size;
   size_t reading_size;

   LARGE_INTEGER offset;

   char flags;

   int pending_writes;
};

static void completion (void * tag, OVERLAPPED * _overlapped,
      unsigned int bytes_transferred, int error)
{
   lw_fdstream ctx = tag;

   lwp_fdstream_overlapped * overlapped =
      (lwp_fdstream_overlapped *) _overlapped;

   if (error == ERROR_OPERATION_ABORTED)
   {
      /* This operation was aborted - we have no guarantee that
       * ctx is valid, so just free overlapped and get out.
       */

      free (overlapped);
      return;
   }

   ++ ctx->ref_count;

   switch (overlapped->type)
   {
      case overlapped_type_read:

         assert (overlapped == ctx->read_overlapped);

         ctx->flags &= ~ lwp_fdstream_flag_read_pending;

         if (error || !bytes_transferred)
         {
            lw_stream_close ((lw_stream) ctx, lw_true);
            break;
         }

         lwp_stream_data ((lw_stream) ctx, ctx->buffer, bytes_transferred);

         /* Calling lwp_stream_data may result in stream destruction */

         if (ctx->flags & lwp_fdstream_flag_dead)
            break;

         issue_read (ctx);
         break;

      case overlapped_type_write:

         free (overlapped);

         if (write_completed (ctx))
            ctx->flags |= lwp_fdstream_flag_dead;

         break;

      case overlapped_type_transmitfile:
      {
         assert (overlapped == ctx->transmit_file_overlapped);

         ctx->transmit_file_from->transmit_file_to = 0;                            
         write_completed (ctx->transmit_file_from);
         ctx->transmit_file_from = 0;

         if (write_completed (ctx))
            ctx->flags |= lwp_fdstream_flag_dead;

         break;
      }
   };

   if ((-- ctx->ref_count) == 0 && (ctx->flags & lwp_fdstream_flag_dead))
      free (ctx);
}

static void close_fd (lw_fdstream ctx)
{
   if (ctx->fd == INVALID_HANDLE_VALUE)
      return;

   if (ctx->transmit_file_from)
   {
      ctx->transmit_file_from->transmit_file_to = 0;
      write_completed (ctx->transmit_file_from);
      ctx->transmit_file_from = 0;

      /* Not calling write_completed for this stream because the FD
       * is closing anyway (and write_completed may result in our
       * destruction, which would be annoying here.)
       */
   }

   if (ctx->transmit_file_to)
   {
      ctx->transmit_file_to->transmit_file_from = 0;
      write_completed (ctx->transmit_file_to);
      ctx->transmit_file_to = 0;
   }

   if (!CancelIo (ctx->fd))
   {
      assert (false);
   }

   ctx->pending_writes = 0;

   if (ctx->flags & lwp_fdstream_flag_auto_close)
   {
      if (ctx->flags & lwp_fdstream_flag_is_socket)
         closesocket ((SOCKET) FD);
      else
         CloseHandle (ctx->fd);
   }

   ctx->fd = INVALID_HANDLE_VALUE;
}

lw_bool write_completed (lw_fdstream ctx)
{
   -- ctx->pending_writes;

   /* If we're trying to close, was this the last write finishing? */

   if ( (ctx->flags & lwp_fdstream_flag_closeASAP) && ctx->pending_writes == 0)
   {
      close_fd (ctx);

      lw_stream_close ((lw_stream) ctx, lw_true);
   }

   return lw_false;
}

static void issue_read (lw_fdstream ctx)
{
   if (ctx->fd == INVALID_HANDLE_VALUE)
      return;

   if (ctx->flags & lwp_fdstream_flag_read_pending)
      return;

   memset (ctx->read_overlapped, 0, sizeof (*ctx->read_overlapped));

   ctx->read_overlapped->type = overlapped_type_read;

   ctx->read_overlapped->overlapped.Offset = ctx->offset.LowPart;
   ctx->read_overlapped->overlapped.OffsetHigh = ctx->offset.HighPart;

   size_t to_read = sizeof (Buffer);

   if (ctx->reading_size != -1 && to_read > ctx->reading_size)
      to_read = ctx->reading_size;

   if (ReadFile (ctx->fd, ctx->buffer, to_read,
                 0, &ctx->read_overlapped->overlapped) == -1)
   {
      int error = GetLastError();

      if (error != ERROR_IO_PENDING)
      {
         lw_stream_close ((lw_stream) ctx);
         return;
      }
   }

   if (ctx->size != -1)
      ctx->offset.QuadPart += to_read;

   ctx->flags |= lwp_fdstream_flag_read_pending;
}

void lw_fdstream_init (lw_fdstream ctx, lw_pump pump)
{
   memset (ctx, 0, sizeof (*ctx));

   ctx->fd     = INVALID_HANDLE_VALUE;
   ctx->flags  = lwp_fdstream_flag_nagle;
   ctx->size   = -1;

   ReadOverlapped = new Overlapped ();
   TransmitFileOverlapped = new Overlapped ();

   lwp_stream_init ((lw_stream) ctx, pump);
}

lw_fdstream lw_fdstream_new (lw_pump pump)
{
   lw_fdstream ctx = malloc (sizeof (*ctx));

   if (!ctx)
      return 0;

   lwp_init ();

   lw_fdstream_init (ctx, pump);

   return ctx;
}

void lw_fdstream_cleanup (lw_fdstream ctx)
{
   lw_stream_close ((lw_stream) ctx, lw_true);

    if (ctx->ref_count > 0)
        ctx->flags |= lwp_fdstream_flag_dead;
    else
       free (ctx);

   close_fd (ctx);

   /* TODO: May leak read_overlapped if no read is pending */
}

void lw_fdstream_delete (lw_fdstream ctx)
{
   if (!ctx)
      return;

   lw_fdstream_cleanup (ctx);

   free (ctx);
}

void lw_fdstream_set_fd (HANDLE fd, lw_pump_watch watch, lw_bool auto_close)
{
   if (ctx->watch)
      lw_pump_remove (ctx->pump, ctx->watch);

   ctx->fd = fd;

   if (fd == INVALID_HANDLE_VALUE)
      return;

   WSAPROTOCOL_INFO info;

   ctx->flags |= lwp_fdstream_flag_is_socket;

   if (auto_close)
      ctx->flags |= lwp_fdstream_flag_auto_close;

    if (WSADuplicateSocket ((SOCKET) ctx->fd,
                            GetCurrentProcessId (),
                            &info) == SOCKET_ERROR)
    {
        int error = WSAGetLastError ();

        if (error == WSAENOTSOCK || error == WSAEINVAL)
            ctx->flags &= ~ lwp_fdstream_flag_is_socket;
    }
 
    if (ctx->flags & lwp_fdstream_flag_is_socket)
    {
       ctx->size = -1;

       int b = (ctx->flags & lwp_fdstream_flag_nagle) ? 0 : 1;

       setsockopt ((SOCKET) FD, SOL_SOCKET, TCP_NODELAY,
                     (char *) &b, sizeof (b));
    }
    else
    {
       LARGE_INTEGER size;

       if (!compat_GetFileSizeEx () (ctx->fd, &size))
          return;

       ctx->size = (size_t) size.QuadPart;

       assert (ctx->size != -1);
    }

    if (watch)
    {
        ctx->watch = watch;
        lw_pump_update_callbacks (ctx->pump, watch, ctx, completion);
    }
    else
    {
       ctx->watch = lw_pump_add (ctx->pump, fd, ctx, completion);
    }

    if (ctx->reading_size != 0)
       issue_read (ctx);
}

lw_bool lw_fdstream_valid (lw_fdstream ctx)
{
    return ctx->fd != INVALID_HANDLE_VALUE;
}

/* Note that this always swallows all of the data (unlike on *nix where it
 * might only be able to use some of it.)  Because Windows FDStream never
 * calls WriteReady(), it's important that nothing gets buffered in the
 * Stream.
 */

static size_t def_sink_data (const char * buffer, size_t size)
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

   overlapped->_Overlapped.Offset = ctx->Offset.LowPart;
   overlapped->_Overlapped.OffsetHigh = ctx->Offset.HighPart;

   /* TODO : Find a way to avoid copying the data. */

   memcpy (overlapped->Data, buffer, size);

   /* TODO : Use WSASend if IsSocket == true?  (for better error messages.)
    * Same goes for ReadFile and WSARecv.
    */

   if (::WriteFile (ctx->FD, overlapped->Data, size,
            0, (OVERLAPPED *) overlapped) == -1)
   {
      int error = GetLastError ();

      if (error != ERROR_IO_PENDING)
      {
         /* TODO : send failed - anything to do here? */

         return size;
      }
   }

   if (ctx->Size != -1)
      ctx->Offset.QuadPart += size;

   ++ ctx->PendingWrites;

   return size;
}

static size_t def_sink_stream (Stream &_stream, size_t size)
{
   lw_fdstream ctx = (lw_fdstream) stream;

    FDStream * stream = (FDStream *) _stream.Cast (::FDStreamType);

    if (!stream)
        return -1;

    if (!lw_stream_valid ((lw_stream) ctx))
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

    if (stream->ctx->flags & lwp_fdstream_flag_socket)
        return -1;

    if (! (ctx->flags & lwp_fdstream_flag_socket))
        return -1;

    if (ctx->transmitfile_from || stream->ctx->transmitfile_to)
        return -1;  /* source or dest stream already performing a TransmitFile */

    Internal::Overlapped * overlapped = ctx->transmitfile_overlapped;
    memset (overlapped, 0, sizeof (*overlapped));

    overlapped->type = overlapped_type_transmitfile;

    /* Use the current offset from the source stream */

    ((OVERLAPPED *) overlapped)->Offset = stream->ctx->offset.LowPart;
    ((OVERLAPPED *) overlapped)->OffsetHigh = stream->ctx->offset.HighPart;

    /* TODO : Can we make use of the head buffers somehow?  Perhaps if data
     * is queued and preventing this TransmitFile from happening immediately,
     * the head buffers could be used to drain it.
     */

    if (!TransmitFile
            ((SOCKET) ctx->FD, stream->ctx->FD,
                (DWORD) size, 0, (OVERLAPPED *) overlapped, 0, 0))
    {
        int error = WSAGetLastError ();

        if (error != WSA_IO_PENDING)
            return -1;
    }

    /* OK, looks like the TransmitFile call succeeded. */

    ctx->transmitfile_from = stream->ctx;
    stream->ctx->transmitfile_to = ctx;

    if (stream->ctx->Size != -1)
        stream->ctx->Offset.QuadPart += size;

    ++ ctx->pending_writes;
    ++ stream->ctx->pending_writes;

    return size;
}

void lw_fdstream_read (lw_fdstream ctx, size_t bytes)
{
   lw_bool was_reading = ctx->reading_size != 0;

   if (bytes == -1)
      ctx->reading_size = lw_stream_bytes_left ((lw_stream) ctx);
   else
      ctx->reading_size += bytes;

   if (!was_reading)
      issue_read (ctx);
}

size_t lw_fdstream_bytes_left (lw_fdstream ctx)
{
   if (!lw_stream_valid ((lw_stream) ctx))
      return -1;

   if (ctx->size == -1)
      return -1;

   return ctx->size - ((size_t) ctx->offset.QuadPart);
}

/* Unlike with *nix, there's the possibility that data might actually be
 * pending in the FDStream, rather than just the Stream.  To account for this,
 * we override Close() to check PendingWrites first.
 */

bool FDStream::Close (bool immediate)
{
   return ctx->Close (immediate);
}

static lw_bool def_close (lw_stream stream, bool immediate)
{
   lw_fdstream ctx = (lw_fdstream) stream;

   if (immediate || PendingWrites == 0)
   {
      ++ ctx->ref_count;

      if (!lw_stream_close ((lw_stream) ctx, immediate))
         return false;

      close_fd (ctx);

      if ((-- ctx->ref_count) == 0 && (ctx->flags & lwp_fdstream_flag_dead))
         free (ctx);

      return lw_true;
   }
   else
   {
      ctx->flags |= lwp_fdstream_flag_closeASAP;
      return lw_false;
   }
}

void lw_fdstream_set_nagle (lw_fdstream ctx, lw_bool enabled)
{
   if (enabled)
      ctx->flags |= lwp_fdstream_flag_nagle;
   else
      ctx->flags &= ~ lwp_fdstream_flag_nagle;

   if (lw_stream_valid ((lw_stream) ctx))
   {
      int b = enabled ? 0 : 1;

      setsockopt ((SOCKET) ctx->fd, SOL_SOCKET,
            TCP_NODELAY, (char *) &b, sizeof (b));
   }
}

/* TODO : Can we do anything here on Windows? */

void lw_fdstream_cork (lw_fdstream ctx)
{
}

void lw_fdstream_uncork (lw_fdstream ctx)
{
}



