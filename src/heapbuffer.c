
/* vim: set et ts=3 sw=3 ft=c:
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

#include "common.h"

void lwp_heapbuffer_init (lwp_heapbuffer ctx)
{
    memset (ctx, 0, sizeof (*ctx));
}

void lwp_heapbuffer_free (lwp_heapbuffer ctx)
{
    free (ctx);
}

lw_bool lwp_heapbuffer_add (lwp_heapbuffer ctx, const char * buffer, size_t length)
{
   /* TODO: discard data before the offset (might save a realloc) */

    size_t new_length;
    lw_bool need_realloc = lw_false;

    if (length == -1)
        length = strlen (buffer);

    new_length = ctx->length + length;

    while (new_length > ctx->allocated)
    {
       ctx->allocated = ctx->allocated > 0 ? (ctx->allocated * 3) : (1024 * 4);
       need_realloc = lw_true;
    }

    if (need_realloc)
    {
        ctx = realloc (ctx, sizeof (*ctx) + ctx->allocated);

        if (!ctx)
           return lw_false;
    }

    memcpy (ctx->buffer + ctx->length, buffer, length);
    ctx->length = new_length;

    return lw_true;
}

void lwp_heapbuffer_addf (lwp_heapbuffer ctx, const char * format, ...)
{
   va_list args;
   va_start (args, format);
    
   char * buffer;
   ssize_t length = lwp_format (&buffer, format, args);
    
   if (ctx->length > 0)
      lwp_heapbuffer_add (ctx, buffer, length);

   va_end (args);
}

void lwp_heapbuffer_reset (lwp_heapbuffer ctx)
{
    ctx->length = ctx->offset = 0;
}

size_t lwp_heapbuffer_length (lwp_heapbuffer ctx)
{
   return ctx->length - ctx->offset;
}

char * lwp_heapbuffer_buffer (lwp_heapbuffer ctx)
{
   return ctx->buffer + ctx->offset;
}

void lwp_heapbuffer_trim_left (lwp_heapbuffer ctx, size_t length)
{
   ctx->offset += length;
}

void lwp_heapbuffer_trim_right (lwp_heapbuffer ctx, size_t length)
{
   ctx->length -= length;
}

