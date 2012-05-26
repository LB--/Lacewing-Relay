
/* vim: set et ts=4 sw=4 ft=cpp:
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

#include "../Common.h"

static void onClientClose (Stream &, void * tag);
static void onClientData (Stream &, void * tag, char * buffer, size_t size);

struct Server::Internal
{
    Lacewing::Server &Server;

    int Socket;

    Lacewing::Pump &Pump;
    
    struct
    {
        HandlerConnect Connect;
        HandlerDisconnect Disconnect;
        HandlerReceive Receive;
        HandlerError Error;

    } Handlers;

    Internal (Lacewing::Server &_Server, Lacewing::Pump &_Pump)
                : Server (_Server), Pump (_Pump)
    {
        Socket  = -1;
        
        memset (&Handlers, 0, sizeof (Handlers));

        Context = 0;
    }

    ~ Internal ()
    {
    }
    
    List <Server::Client::Internal *> Clients;

    SSL_CTX * Context;
    char Passphrase [128];
};
    
struct Server::Client::Internal
{
    Lacewing::Server::Client Public;

    Lacewing::Server::Internal &Server;

    Internal (Lacewing::Server::Internal &_Server, int _FD)
        : Server (_Server), FD (_FD), Public (_Server.Pump, _FD)
    {
        Disconnecting = false;

        Public.internal = this;
        Public.Tag = 0;

        /* The first added close handler is always the last called.
         * This is important, because ours will destroy the client.
         */

        Public.AddHandlerClose (onClientClose, &Server);

        if (Server.Context)
        {
            /* TODO : negates the std::nothrow when accepting a client */

            SSL = new SSLClient (Server.Context);
            
            Public.AddFilterDownstream (SSL->Downstream);
            Public.AddFilterUpstream (SSL->Upstream);

            DebugOut ("SSL filters added");
        }
        else
        {   SSL = 0;
        }
    }

    ~ Internal ()
    {
        DebugOut ("Terminate %d", &Public);

        FD = -1;
        
        if (Element) /* connect handler already called? */
        {
            if (Server.Handlers.Disconnect)
                Server.Handlers.Disconnect (Server.Server, Public);

            Server.Clients.Erase (Element);
        }
    }

    bool Disconnecting;

    SSLClient * SSL;

    AddressWrapper Address;

    List <Server::Client::Internal *>::Element * Element;

    int FD;
    void * GoneKey;
};

Server::Client::Client (Lacewing::Pump &Pump, int FD) : FDStream (Pump)
{
    SetFD (FD);
}

Server::Server (Lacewing::Pump &Pump)
{
    LacewingInitialise();
    
    internal = new Server::Internal (*this, Pump);
    Tag = 0;
}

Server::~Server ()
{
    Unhost();

    delete internal;
}

static void ListenSocketReadReady (void * tag)
{
    Server::Internal * internal = (Server::Internal *) tag;

    sockaddr_storage Address;
    socklen_t AddressLength = sizeof(Address);
    
    for(;;)
    {
        int FD;
            
        DebugOut ("Trying to accept...");

        if ((FD = accept
            (internal->Socket, (sockaddr *) &Address, &AddressLength)) == -1)
        {
            DebugOut ("Failed to accept: %s", strerror (errno));
            break;
        }
        
        DebugOut ("Accepted FD %d", FD);

        Server::Client::Internal * client
                = new (std::nothrow) Server::Client::Internal (*internal, FD);

        if (!client)
        {
            DebugOut ("Failed allocating client");
            break;
        }

        client->Address.Set (&Address);

        if (internal->Handlers.Connect)
            internal->Handlers.Connect (internal->Server, client->Public);
        
        /* Did the client get disconnected by the connect handler? */

        if (! ((FDStream *) client)->Valid () )
        {
            delete client;
            continue;
        }

        /* TODO : What if the connect handler called disconnect? */

        client->Element = internal->Clients.Push (client);

        if (internal->Handlers.Receive)
        {
            DebugOut ("*** READING on behalf of the handler, client tag %p",
                            client->Public.Tag);

            client->Public.AddHandlerData (onClientData, internal);
            client->Public.Read (-1);
        }
    }
}

void Server::Host (int Port)
{
    Filter Filter;
    Filter.LocalPort(Port);

    Host(Filter);
}

void Server::Host (Filter &Filter)
{
    Unhost();
    
    {   Lacewing::Error Error;

        if ((internal->Socket = CreateServerSocket
                (Filter, SOCK_STREAM, IPPROTO_TCP, Error)) == -1)
        {
            if (internal->Handlers.Error)
                internal->Handlers.Error (*this, Error);

            return;
        }
    }

    if (listen (internal->Socket, SOMAXCONN) == -1)
    {
        Error Error;
        
        Error.Add(errno);
        Error.Add("Error listening");

        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);
        
        return;
    }

    internal->Pump.Add
        (internal->Socket, internal, ListenSocketReadReady);
}

void Server::Unhost ()
{
    if(!Hosting ())
        return;

    close (internal->Socket);
    internal->Socket = -1;
}

bool Server::Hosting ()
{
    return internal->Socket != -1;
}

int Server::ClientCount ()
{
    return internal->Clients.Size;
}

int Server::Port ()
{
    return GetSocketPort (internal->Socket);
}

bool Server::CertificateLoaded ()
{
    return internal->Context != 0;
}

static int PasswordCallback (char * Buffer, int, int, void * Tag)
{
    Server::Internal * internal = (Server::Internal *) Tag;

    strcpy (Buffer, internal->Passphrase);
    return strlen (internal->Passphrase);
}

bool Server::LoadCertificateFile
        (const char * Filename, const char * Passphrase)
{
    SSL_load_error_strings();

    internal->Context = SSL_CTX_new (SSLv23_server_method ());

    LacewingAssert (internal->Context);

    strcpy (internal->Passphrase, Passphrase);

    SSL_CTX_set_mode (internal->Context,
        SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER

        #if HAVE_DECL_SSL_MODE_RELEASE_BUFFERS
             | SSL_MODE_RELEASE_BUFFERS
        #endif
    );

    SSL_CTX_set_quiet_shutdown (internal->Context, 1);

    SSL_CTX_set_default_passwd_cb (internal->Context, PasswordCallback);
    SSL_CTX_set_default_passwd_cb_userdata (internal->Context, internal);

    if (SSL_CTX_use_certificate_chain_file (internal->Context, Filename) != 1)
    {
        DebugOut("Failed to load certificate chain file: %s",
                        ERR_error_string(ERR_get_error(), 0));

        internal->Context = 0;
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file
            (internal->Context, Filename, SSL_FILETYPE_PEM) != 1)
    {
        DebugOut("Failed to load private key file: %s",
                        ERR_error_string(ERR_get_error(), 0));

        internal->Context = 0;
        return false;
    }

    return true;
}

bool Server::LoadSystemCertificate (const char *, const char *, const char *)
{
    Error Error;
    Error.Add("System certificates are only supported on Windows");

    if (internal->Handlers.Error)
        internal->Handlers.Error (*this, Error);

    return false;
}

Address &Server::Client::GetAddress ()
{
    return internal->Address;
}

Server::Client * Server::Client::Next ()
{
    return internal->Element->Next ?
        &(** internal->Element->Next)->Public : 0;
}

Server::Client * Server::FirstClient ()
{
    return internal->Clients.First ?
            &(** internal->Clients.First)->Public : 0;
}

void onClientData (Stream &stream, void * tag, char * buffer, size_t size)
{
    Server::Internal * internal = (Server::Internal *) tag;

    internal->Handlers.Receive
        (internal->Server, *(Server::Client *) &stream, buffer, size);
}

void onClientClose (Stream &stream, void * tag)
{
    Server::Client::Internal * client = (Server::Client::Internal *) &stream;

    /* If the client doesn't have a list element, we're still inside
     * the connect handler (so shouldn't delete the client)
     */

    if (!client->Element)
        return;

    delete client;
}

void Server::onReceive (Server::HandlerReceive onReceive)
{
    internal->Handlers.Receive = onReceive;

    if (onReceive)
    {
        /* Setting onReceive to a handler */

        if (!internal->Handlers.Receive)
        {
            for (List <Server::Client::Internal *>::Element * E
                    = internal->Clients.First; E; E = E->Next)
            {
                (** E)->Public.AddHandlerData (onClientData, internal);
                (** E)->Public.Read (-1);
            }
        }
        
        return;
    }

    /* Setting onReceive to 0 */

    for (List <Server::Client::Internal *>::Element * E
            = internal->Clients.First; E; E = E->Next)
    {
        (** E)->Public.RemoveHandlerData (onClientData, internal);
    }
}

AutoHandlerFunctions (Server, Connect)
AutoHandlerFunctions (Server, Disconnect)
AutoHandlerFunctions (Server, Error)

