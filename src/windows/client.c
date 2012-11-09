
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
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

struct lw_client
{
   lw_pump pump;
   lw_pump_watch watch;

   lwp_client_hook_connect     on_connect;
   lwp_client_hook_disconnect  on_disconnect;
   lwp_client_hook_receive     on_receive;
   lwp_client_hook_error       on_error;

   lw_addr server_addr;

   HANDLE socket;
   lw_bool connecting;
};

lw_client lw_client_new (lw_pump pump)
{
   lwp_init ();

   lw_client ctx = calloc (sizeof (*ctx), 1);

   ctx->socket = INVALID_HANDLE_VALUE;
   
   return ctx;
}

void lw_client_delete (lw_client ctx)
{
   if (!ctx)
      return;

   lw_addr_delete (ctx->server_addr);

   free (ctx);
}

void lw_client_connect (lw_client ctx, const char * host, long port)
{
   lw_addr addr = lw_addr_new (host, port, lw_true);
   lw_client_connect_addr (ctx, addr);
}

static void completion (void * tag, OVERLAPPED * overlapped,
                            unsigned int bytes_transfered, int error)
{
   lw_client ctx = tag;

   assert (ctx->connecting);

   if(error)
   {
      ctx->connecting = lw_false;

      Lacewing::Error Error;

      Error.Add(error);
      Error.Add("Error connecting");

      if (ctx->Handlers.Error)
         ctx->Handlers.Error (ctx->Public, Error);

      return;
   }

   ctx->Public.SetFD (ctx->Socket, ctx->Watch);
   ctx->Connecting = false;

   if (ctx->Handlers.Connect)
      ctx->Handlers.Connect (ctx->Public);

   if (ctx->Handlers.Receive)
      ctx->Public.Read (-1);
}

void Client::Connect (Address &Address)
{
    if (Connected () || Connecting ())
    {
        Lacewing::Error Error;
        Error.Add("Already connected to a server");
        
        if (ctx->Handlers.Error)
            ctx->Handlers.Error (*this, Error);

        return;
    }

    ctx->Connecting = true;

    /* TODO : Resolve asynchronously? */

    {   Error * Error = Address.Resolve ();

        if (Error)
        {
            if (ctx->Handlers.Error)
                ctx->Handlers.Error (*this, *Error);
                
            return;
        }
    }

    if (!Address.ctx->Info)
    {
        Lacewing::Error Error;
        Error.Add ("Invalid address");
        
        if (ctx->Handlers.Error)
            ctx->Handlers.Error (*this, Error);

        return;
    }

    if ((ctx->Socket = (HANDLE) WSASocket
            (Address.IPv6 () ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP,
                    0, 0, WSA_FLAG_OVERLAPPED)) == INVALID_HANDLE_VALUE)
    {
        Lacewing::Error Error;
       
        Error.Add(WSAGetLastError ());        
        Error.Add("Error creating socket");
        
        if (ctx->Handlers.Error)
            ctx->Handlers.Error (*this, Error);

        return;
    }

    lwp_disable_ipv6_only ((lwp_socket) ctx->Socket);

    ctx->Watch = ctx->Pump.Add
        (ctx->Socket, ctx, ::Completion);

    /* LPFN_CONNECTEX and WSAID_CONNECTEX aren't defined w/ MinGW */

    static BOOL (PASCAL FAR * lw_ConnectEx)
    (   
        SOCKET s,
        const struct sockaddr FAR *name,
        int namelen,
        PVOID lpSendBuffer,
        DWORD dwSendDataLength,
        LPDWORD lpdwBytesSent,
        LPOVERLAPPED lpOverlapped
    );

    GUID ID = {0x25a207b9,0xddf3,0x4660,
                        {0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}};

    DWORD bytes = 0;

    WSAIoctl ((SOCKET) ctx->Socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                    &ID, sizeof (ID), &lw_ConnectEx, sizeof (lw_ConnectEx),
                        &bytes, 0, 0);

    assert (lw_ConnectEx);

    sockaddr_storage LocalAddress;
        memset (&LocalAddress, 0, sizeof (LocalAddress));

    if (Address.IPv6 ())
    {
        ((sockaddr_in6 *) &LocalAddress)->sin6_family = AF_INET6;
        ((sockaddr_in6 *) &LocalAddress)->sin6_addr = in6addr_any;       
    }
    else
    {
        ((sockaddr_in *) &LocalAddress)->sin_family = AF_INET;
        ((sockaddr_in *) &LocalAddress)->sin_addr.S_un.S_addr = INADDR_ANY;
    }

    if (bind ((SOCKET) ctx->Socket,
            (sockaddr *) &LocalAddress, sizeof (LocalAddress)) == -1)
    {
        Lacewing::Error Error;
       
        Error.Add (WSAGetLastError ());        
        Error.Add ("Error binding socket");
        
        if (ctx->Handlers.Error)
            ctx->Handlers.Error (*this, Error);

        return;
    }

    delete ctx->Address;
    ctx->Address = new Lacewing::Address (Address);

    OVERLAPPED * overlapped = new OVERLAPPED;
    memset (overlapped, 0, sizeof (OVERLAPPED));

    if (!lw_ConnectEx ((SOCKET) ctx->Socket, Address.ctx->Info->ai_addr,
                        Address.ctx->Info->ai_addrlen, 0, 0, 0, overlapped))
    {
        int Code = WSAGetLastError ();

        assert(Code == WSA_IO_PENDING);
    }
}

bool Client::Connected ()
{
    return Valid ();
}

bool Client::Connecting ()
{
    return ctx->Connecting;
}

Address &Client::ServerAddress ()
{
    return *ctx->Address;
}

static void onData (Stream &, void * tag, const char * buffer, size_t size)
{
    Client::Internal * ctx = (Client::Internal *) tag;

    ctx->Handlers.Receive (ctx->Public, buffer, size);
}

void Client::onReceive (Client::HandlerReceive onReceive)
{
    ctx->Handlers.Receive = onReceive;

    if (onReceive)
    {
        AddHandlerData (::onData, ctx);
        Read (-1);
    }
    else
    {
        RemoveHandlerData (::onData, ctx);
    }
}

static void onClose (Stream &, void * tag)
{
    Client::Internal * ctx = (Client::Internal *) tag;

    ctx->Handlers.Disconnect (ctx->Public);
}

void Client::onDisconnect (Client::HandlerDisconnect onDisconnect)
{
    ctx->Handlers.Disconnect = onDisconnect;

    if (onDisconnect)
        AddHandlerClose (::onClose, ctx);
    else
        RemoveHandlerClose (::onClose, ctx);
}

AutoHandlerFunctions (Client, Connect)
AutoHandlerFunctions (Client, Error)

