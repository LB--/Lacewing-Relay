
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
#include "winsslclient.h"

static void on_client_close (lw_stream, void * tag);
static void on_client_data (lw_stream, void * tag, const char * buffer, size_t size);

struct lw_server
{
   SOCKET socket;

   lw_pump pump;

   lw_server_hook_connect on_onnect;
   lw_server_hook_disconnect on_disconnect;
   lw_server_hook_receive on_receive;
   lw_server_hook_error on_error;

   lw_server_client first_client;

   lw_bool cert_loaded;
   CredHandle ssl_creds;

   long accepts_posted;
};
    
lw_server lw_server_new (lw_pump pump)
{
   lw_server ctx = calloc (sizeof (*ctx), 1);

   if (!ctx)
      return 0;

   lwp_init ();

   ctx->socket = -1;
   ctx->pump = pump;

   return ctx;
}

void lw_server_delete (lw_server ctx)
{
   lw_server_unhost (ctx);

   free (ctx);
}

struct lw_server_client
{
    int user_count;
    lw_bool dead;

    lwp_winsslclient ssl;

    struct lw_addr addr;

    HANDLE socket;
};

lw_server_client lwp_server_client_new (lw_server ctx)
{
   lw_server_client client = calloc (sizeof (*client), 1);

   /* The first added close handler is always the last called.
    * This is important, because ours will destroy the client.
    */

   lw_stream_add_hook_close ((lw_stream) client, client);

   if (ctx->cert_loaded)
   {
      client->ssl = lwp_winsslclient_new (ctx->ssl_creds);

      lw_stream_addfilter_downstream
         ((lw_stream) client, client->ssl->downstream, lw_false, lw_true);

      lw_stream_addfilter_upstream
         ((lw_stream) client, client->ssl->upstream, lw_false, lw_true);
   }
}

void lwp_server_client_delete (lw_server_client client)
{
   lwp_trace ("Terminate %p", client);

   ++ client->user_count;

   client->socket = INVALID_HANDLE_VALUE;

   if (client->connect_hook_called)
   {
      if (ctx->on_disconnect)
         ctx->on_disconnect (ctx, client);

      Server.Clients.Erase (Element);
   }

   lwp_winsslclient_delete (client->ssl);

   free (client);
}

const int ideal_pending_accept_count = 16;

typedef struct 
{
   OVERLAPPED overlapped;

   HANDLE socket;

   struct lw_addr addr;
   char addr_buffer [(sizeof (sockaddr_storage) + 16) * 2];

} accept_overlapped;

static lw_bool issue_accept (lw_server ctx)
{
   accept_overlapped * overlapped = calloc (sizeof (*overlapped), 1);

   if (!overlapped)
      return lw_false;

   if ((overlapped->socket = (HANDLE) WSASocket
            (lwp_socket_addr (socket).ss_family, SOCK_STREAM, IPPROTO_TCP,
             0, 0, WSA_FLAG_OVERLAPPED)) == INVALID_HANDLE_VALUE)
   {
      free (overlapped);
      return lw_false;
   }

   lwp_disable_ipv6_only ((lwp_socket) overlapped->FD);

   DWORD bytes_received; 

   /* TODO : Use AcceptEx to receive the first data? */

   if (!AcceptEx (socket, (SOCKET) overlapped->socket, overlapped->addr_buffer,
            0, sizeof (sockaddr_storage) + 16, sizeof (sockaddr_storage) + 16,
            &bytes_received, (OVERLAPPED *) overlapped))
   {
      int error = WSAGetLastError ();

      if (error != ERROR_IO_PENDING)
         return false;
   }

   ++ ctx->accepts_posted;

   return lw_true;
}

static void listen_socket_completion (void * tag, OVERLAPPED * _overlapped,
                                      unsigned int bytes_transferred, int error)
{
   lw_server ctx = tag;
   accept_overlapped * overlapped = (accept_overlapped *) _overlapped;

   -- ctx->accepts_posted;

   if (error)
   {
      free (overlapped);
      return;
   }

   while (ctx->accepts_posted < ideal_pending_accept_count)
      if (!issue_accept (ctx))
         break;

   setsockopt ((SOCKET) overlapped->FD, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                  (char *) &ctx->socket, sizeof (ctx->socket));

   sockaddr_storage * local_addr, * remote_addr;
   int local_addr_len, remote_addr_len;

   GetAcceptExSockaddrs
   (
      overlapped->addr_buffer,
      0,

      sizeof (struct sockaddr_storage) + 16,
      sizeof (struct sockaddr_storage) + 16,

      (sockaddr **) &local_addr,
      &local_addr_len,

      (sockaddr **) &remote_addr,
      &remote_addr_len
   );

   lw_server_client client = lwp_server_client_new (ctx, overlapped->socket);

   if (!client)
   {
      closesocket ((SOCKET) overlapped->socket);
      free (overlapped);

      return;
   }

   lw_addr_set (client->addr, remote_addr);
   free (overlapped);

   ++ client->user_count;

   if (ctx->on_connect)
      ctx->on_connect (ctx, client);

   if (client->dead)
   {
      lwp_server_client_delete (client);
      return;
   }

   client->Element = ctx->Clients.Push (client);

   -- client->user_count;

   if (ctx->on_receive)
   {
      client->Public.AddHandlerData (onClientData, client);
      client->Public.Read (-1);
   }
}

void lw_server_host (lw_server ctx, long port)
{
   lw_filter filter = lw_filter_new ();
   lw_filter_set_local_port (filter, port);

   lw_server_host_filter (ctx, filter);
}

void lw_server_host_filter (lw_server ctx, lw_filter filter)
{
   lw_server_unhost (server);

   lw_error error = lw_error_new ();

   if ((ctx->socket = lwp_create_server_socket
            (filter, SOCK_STREAM, IPPROTO_TCP, error)) == -1)
   {
      if (ctx->on_error)
         ctx->on_error (ctx, error);

      return;
   }

   if (listen (ctx->socket, SOMAXCONN) == -1)
   {
      Error.Add (WSAGetLastError ());
      Error.Add ("Error listening");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      return;
   }

   lw_pump_add (ctx->pump, (HANDLE) ctx->socket, ctx, listen_socket_completion);

   while (ctx->accepts_posted < ideal_pending_accept_count)
      if (!issue_accept (ctx))
         break;
}

void lw_server_unhost (lw_server ctx)
{
    if(!lw_server_hosting (ctx))
        return;

    closesocket (ctx->socket);
    ctx->socket = -1;
}

lw_bool lw_server_hosting (lw_server ctx)
{
   return ctx->socket != -1;
}

size_t lw_server_num_clients (lw_server ctx)
{
   return ctx->num_clients;
}

long lw_server_port (lw_server ctx)
{
   return lwp_socket_port (ctx->socket);
}

lw_bool lw_server_load_sys_cert (lw_server ctx,
                                 const char * store_name,
                                 const char * common_name,
                                 const char * location)
{
   if (lw_server_hosting (ctx) || lw_server_cert_loaded (ctx))
   {
      lw_error error = lw_error_new ();

      lw_error_addf (error,
            "Either the server is already hosting, or a certificate has already been loaded");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return false;
   }

   if(!location || !*location)
      location = "CurrentUser";

   if(!store_name || !*store_name)
      store_name = "MY";

   int location_id = -1;

   do
   {
      if(!strcasecmp (location, "CurrentService"))
      {
         location_id = 0x40000; /* CERT_SYSTEM_STORE_CURRENT_SERVICE */
         break;
      }

      if(!strcasecmp (location, "CurrentUser"))
      {
         location_id = 0x10000; /* CERT_SYSTEM_STORE_CURRENT_USER */
         break;
      }

      if(!strcasecmp (location, "CurrentUserGroupPolicy"))
      {
         location_id = 0x70000; /* CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY */
         break;
      }

      if(!strcasecmp (location, "LocalMachine"))
      {
         location_id = 0x20000; /* CERT_SYSTEM_STORE_LOCAL_MACHINE */
         break;
      }

      if(!strcasecmp (location, "LocalMachineEnterprise"))
      {
         location_id = 0x90000; /* CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE */
         break;
      }

      if(!strcasecmp (location, "LocalMachineGroupPolicy"))
      {
         location_id = 0x80000; /* CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY */
         break;
      }

      if(!strcasecmp (location, "Services"))
      {
         location_id = 0x50000; /* CERT_SYSTEM_STORE_SERVICES */
         break;
      }

      if(!strcasecmp (location, "Users"))
      {
         location_id = 0x60000; /* CERT_SYSTEM_STORE_USERS */
         break;
      }

   } while(0);
    
   if (location_id == -1)
   {
      lw_error error = lw_error_new ();

      lw_error_addf (error, "Unknown certificate location: %s", location);
      lw_error_addf (error, "Error loading certificate");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      return lw_false;
   }

   HCERTSTORE cert_store = CertOpenStore
   (
      (LPCSTR) 9, /* CERT_STORE_PROV_SYSTEM_A */
      0,
      0,
      location_id,
      store_name
   );

   if (!cert_store)
   {
      Lacewing::Error Error;

      lw_error error = lw_error_new ();

      lw_error_add (error, WSAGetLastError ());
      lw_error_addf (error, "Error loading certificate");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return lw_false;
   }

   PCCERT_CONTEXT context = CertFindCertificateInStore
   (
      cert_store,
      X509_ASN_ENCODING,
      0,
      CERT_FIND_SUBJECT_STR_A,
      common_name,
      0
   );

   if (!context)
   {
      int code = GetLastError();

      context = CertFindCertificateInStore
      (
         CertStore,
         PKCS_7_ASN_ENCODING,
         0,
         CERT_FIND_SUBJECT_STR_A,
         CommonName,
         0
      );

      if (!context)
      {
         lw_error error = lw_error_new ();

         lw_error_add (error, code);
         lw_error_addf (error, "Error finding certificate in store");

         if (ctx->on_error)
            ctx->on_error (ctx, error);

         lw_error_delete (error);

         return lw_false;
      }
   }

   SCHANNEL_CRED creds =
   {
      .dwVersion              = SCHANNEL_CRED_VERSION;
      .cCreds                 = 1;
      .paCred                 = &Context;
      .grbitEnabledProtocols  = (0x80 | 0x40); /* SP_PROT_TLS1 */
   };

   {  TimeStamp expiry_time;

      int Result = AcquireCredentialsHandleA
      (
         0,
         (SEC_CHAR *) UNISP_NAME_A,
         SECPKG_CRED_INBOUND,
         0,
         &creds,
         0,
         0,
         &ctx->ssl_creds,
         &expiry_time
      );

      if (result != SEC_E_OK)
      {
         lw_error error = lw_error_new ();

         lw_error_add (error, result);
         lw_error_addf (error, "Error acquiring credentials handle");

         if (ctx->on_error)
            ctx->on_error (ctx, error);

         lw_error_delete (error);

         return lw_false;
      }
   }

   ctx->cert_loaded = lw_true;

   return lw_true;
}

lw_bool lw_server_load_cert_file (lw_server ctx,
                                  const char * filename,
                                  const char * common_name)
{
   if (!lw_file_exists (filename))
   {
      lw_error error = lw_error_new ();

      lw_error_addf (error, "File not found: %s", filename);
      lw_error_addf (error, "Error loading certificate");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return lw_false;
    }

   if (lw_server_hosting (ctx))
      lw_server_unhost (ctx);

   if(lw_server_cert_loaded (ctx))
   {
      FreeCredentialsHandle (&ctx->ssl_creds);
      ctx->cert_loaded = lw_false;
   }

   HCERTSTORE CertStore = CertOpenStore
   (
      (LPCSTR) 7 /* CERT_STORE_PROV_FILENAME_A */,
      X509_ASN_ENCODING,
      0,
      CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG,
      filename
   );

   lw_bool pkcs7 = lw_false;

   if (!cert_store)
   {
      cert_store = CertOpenStore
      (
         (LPCSTR) 7 /* CERT_STORE_PROV_FILENAME_A */,
         PKCS_7_ASN_ENCODING,
         0,
         CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG,
         filename
      );

      pkcs7 = lw_true;

      if (!cert_store)
      {
         lw_error error = lw_error_new ();

         lw_error_add (GetLastError ());
         lw_error_addf (error, "Error loading certificate file: %s", filename);

         if (ctx->on_error)
            ctx->on_error (ctx, error);

         lw_error_delete (error);

         return lw_false;
      }
   }

   PCCERT_CONTEXT context = CertFindCertificateInStore
   (
      cert_store,
      PKCS7 ? PKCS_7_ASN_ENCODING : X509_ASN_ENCODING,
      0,
      CERT_FIND_SUBJECT_STR_A,
      common_name,
      0
   );

   if (!context)
   {
      int code = GetLastError();

      context = CertFindCertificateInStore
      (
         cert_store,
         pkcs7 ? X509_ASN_ENCODING : PKCS_7_ASN_ENCODING,
         0,
         CERT_FIND_SUBJECT_STR_A,
         common_name,
         0
      );

      if (!context)
      {
         lw_error error = lw_error_new ();

         lw_error_add (code);
         lw_error_addf (error, "Error finding certificate in store");

         if (ctx->on_error)
            ctx->on_error (ctx, error);

         lw_error_delete (error);

         return lw_false;
      }
   }

   SCHANNEL_CRED creds =
   {
      .dwVersion = SCHANNEL_CRED_VERSION,
      .cCreds = 1,
      .paCred = &Context,
      .grbitEnabledProtocols = 0xF0 /* SP_PROT_SSL3TLS1 */
   };

   TimeStamp expiry_time; 

   int result = AcquireCredentialsHandleA
   (
      0,
      (SEC_CHAR *) UNISP_NAME_A,
      SECPKG_CRED_INBOUND,
      0,
      &creds,
      0,
      0,
      &ctx->ssl_creds,
      &expiry_time
   );

   if (result != SEC_E_OK)
   {
      lw_error error = lw_error_new ();

      lw_error_add (result);
      lw_error_addf (error, "Error acquiring credentials handle");

      if (ctx->on_error)
         ctx->on_error (ctx, error);

      lw_error_delete (error);

      return lw_false;
   }

   ctx->cert_loaded = lw_true;

   return lw_true;
}

lw_bool lw_server_cert_loaded (lw_server ctx)
{
   return ctx->cert_loaded;   
}

lw_bool lw_server_can_npn (lw_server ctx)
{
    /* NPN is currently not available w/ schannel */

    return lw_false;
}

void lw_server_add_npn (lw_server ctx, const char *)
{
}

const char * lw_server_client_npn (lw_server_client client)
{
    return "";
}

lw_addr lw_server_client_get_addr (lw_server_client client)
{
    return client->addr;
}

lw_server_client lw_server_client_next (lw_server_client client)
{

}

lw_server_client lw_server_first_client (lw_server ctx) 
{

}

void onClientData (Stream &stream, void * tag, const char * buffer, size_t size)
{
    Server::Client::Internal * client = (Server::Client::Internal *) tag;
    Server::Internal &server = client->Server;

    server.Handlers.Receive (server.Public, client->Public, buffer, size);
}

void onClientClose (Stream &stream, void * tag)
{
    Server::Client::Internal * client = (Server::Client::Internal *) tag;

    if (client->UserCount > 0)
        client->Dead = false;
    else
        delete client;
}

void Server::onReceive (Server::HandlerReceive onReceive)
{
    ctx->Handlers.Receive = onReceive;
    
    if (onReceive)
    {
        /* Setting onReceive to a handler */

        if (!ctx->Handlers.Receive)
        {
            for (List <Server::Client::Internal *>::Element * E
                    = ctx->Clients.First; E; E = E->Next)
            {
                (** E)->Public.AddHandlerData (onClientData, (** E));
            }
        }
        
        return;
    }

    /* Setting onReceive to 0 */

    for (List <Server::Client::Internal *>::Element * E
            = ctx->Clients.First; E; E = E->Next)
    {
        (** E)->Public.RemoveHandlerData (onClientData, (** E));
    }
}

AutoHandlerFunctions (Server, Connect)
AutoHandlerFunctions (Server, Disconnect)
AutoHandlerFunctions (Server, Error)

