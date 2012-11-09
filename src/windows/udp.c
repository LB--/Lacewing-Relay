
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012 James McLaughlin et al.  All rights reserved.
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
#include "../address.h"

const int ideal_pending_receive_count = 16;

enum overlapped_type
{
   overlapped_type_send,
   overlapped_type_receive
};

struct udp_overlapped
{
    OVERLAPPED overlapped;

    overlapped_type type;
    void * tag;
};

typedef struct 
{
    char buffer [lwp_default_buffer_size];
    WSABUF winsock_buffer;

    struct sockaddr_storage from;
    size_t from_length;

} udp_receive_info;

udp_receive_info * udp_receive_info_new ()
{
   udp_receive_info * info = malloc (sizeof (*info));

   info->winsock_buffer.buf = info->buffer;
   info->winsock_buffer.len = sizeof (info->buffer);

   info->from_length = sizeof (info->from);

   return info;
}

struct lw_udp
{
   lw_pump pump;

   lw_udp_hook_receive on_receive;
   lw_udp_hook_error on_error;

   lw_filter filter;

   long port;

   SOCKET socket;

   long receives_posted;
};

static void post_receives (lw_udp ctx)
{
   while (ctx->receives_posted < ideal_pending_receive_count)
   {
      udp_receive_info receive_info = udp_receive_info_new ();

      if (!receive_info)
         break;

      udp_overlapped * overlapped = calloc (sizeof (*overlapped), 1);

      if (!overlapped)
         break;

      overlapped->type = overlapped_type_receive;
      overlapped->tag = receive_info;

      DWORD flags = 0;

      if (WSARecvFrom (ctx->socket, &receive_info->winsock_buffer,
                      1, 0, &flags, (sockaddr *) &receive_info->from,
                      &receive_info->from_length,
                      (OVERLAPPED *) overlapped, 0) == -1)
      {
         int error = WSAGetLastError();

         if (error != WSA_IO_PENDING)
            break;
      }

      ++ ctx->receives_posted;
   }
}

static void udp_socket_completion (lw_udp ctx, udp_overlapped * overlapped,
      unsigned int bytes_transferred, int error)
{
   switch (overlapped->type)
   {
      case overlapped_type_send:
      {
         break;
      }

      case overlapped_type_receive:
      {
         udp_receive_info * info = overlapped->tag;

         info->buffer [bytes_transferred] = 0;

         lw_addr addr = lw_addr_new ();
         lw_addr_set (addr, &info->from);

         lw_addr filter_addr = lw_filter_remote (ctx->filter);

         if (filter_addr && !lw_addr_equal (addr, filter_addr))
            break;

         if (ctx->on_receive)
            ctx->on_receive (ctx, addr, info->buffer, bytes_transferred);

         free (info);

         -- ctx->receives_posted;
         post_receives (ctx);

         break;
      }
   };

   free (overlapped);
}

void lw_udp_host (lw_udp ctx, long port)
{
   lw_filter filter = lw_filter_new ();
   lw_filter_set_local_port (filter, port);

   lw_udp_host_filter (filter);

   lw_filter_delete (filter);
}

void lw_udp_host_addr (lw_udp ctx, lw_addr addr)
{
   lw_filter filter = lw_filter_new ();
   lw_filter_set_remote (addr);

   lw_udp_host_filter (filter);

   lw_filter_delete (filter);
}

void lw_udp_host_filter (lw_udp ctx, lw_filter filter)
{
   lw_udp_unhost (ctx);

   lw_error error;

   if ((ctx->fd = lwp_create_server_socket
            (filter, SOCK_DGRAM, IPPROTO_UDP, error)) == -1)
   {
      if (ctx->on_error)
         ctx->on_error (ctx, error);

      return;
   }

   lw_filter_delete (ctx->filter);
   ctx->filter = lw_filter_new ();

   lw_pump_add (ctx->pump, ctx->socket, ctx, udp_socket_completion);

   ctx->port = lwp_socket_port (ctx->socket);

   post_receives (ctx);
}

lw_bool lw_udp_hosting (lw_udp ctx)
{
    return ctx->socket != -1;
}

void lw_udp_unhost (lw_udp ctx)
{
    lwp_close_socket (ctx->socket);
    ctx->socket = -1;
}

long lw_udp_port (lw_udp ctx)
{
    return ctx->port;
}

void lw_udp_write (lw_addr addr, const char * buffer, size_t size)
{
   if ((!lw_addr_ready (addr)) || !address->info)
   {
      lw_error error = lw_error_new ();

      lw_error_addf (error, "The address object passed to write() wasn't ready");
      lw_error_addf (error, "Error sending datagram");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return;
   }

   if (size == -1)
      size = strlen (buffer);

   WSABUF buffer = { size, (CHAR *) buffer };

   udp_overlapped * overlapped = udp_overlapped_new ();
    
   overlapped->type = overlapped_type_send;
   overlapped->tag = 0;

   if (!addr->info)
      return;

   if (WSASendTo (ctx->socket, &buffer, 1, 0, 0, addr->info->ai_addr,
                  addr->info->ai_addrlen, (OVERLAPPED *) overlapped, 0) == -1)
   {
      int code = WSAGetLastError();

      if (code != WSA_IO_PENDING)
      {
         lw_error error = lw_error_new ();

         lw_error_add (error, code) wasn't ready");
         lw_error_addf (error, "Error sending datagram");

         if (ctx->on_error)
            ctx->on_error (ctx, error);

         lw_error_delete (error);

         return;
      }
   }
}

lw_def_hook (udp, error)
lw_def_hook (udp, receive)

