
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

#include "../lw_common.h"
#include "../openssl/SSLClient.h"
#include "../Address.h"

static void onClientClose (Stream &, void * tag);
static void onClientData (Stream &, void * tag, const char * buffer, size_t size);

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

        #ifdef LacewingNPN
            *NPN = 0;
        #endif
    }

    ~ Internal ()
    {
    }
    
    List <Server::Client::Internal *> Clients;

    SSL_CTX * Context;
    char Passphrase [128];

    #ifdef LacewingNPN
        unsigned char NPN [128];
    #endif
};
    
struct Server::Client::Internal
{
    Lacewing::Server::Client Public;

    Lacewing::Server::Internal &Server;

    Internal (Lacewing::Server::Internal &_Server, int _FD)
        : Server (_Server), FD (_FD), Public (_Server.Pump, _FD)
    {
        UserCount = 0;
        Dead = false;

        Public.internal = this;
        Public.Tag = 0;

        Element = 0;

        /* The first added close handler is always the last called.
         * This is important, because ours will destroy the client.
         */

        Public.AddHandlerClose (onClientClose, this);

        if (Server.Context)
        {
            /* TODO : negates the std::nothrow when accepting a client */

            SSL = new SSLClient (Server.Context);

            SSL->tag = this;
            SSL->onHandshook = onSSLHandshook;
            
            Public.AddFilterDownstream (SSL->Downstream, false, true);
            Public.AddFilterUpstream (SSL->Upstream, false, true);

            lwp_trace ("SSL filters added");
        }
        else
        {   SSL = 0;
        }
    }

    ~ Internal ()
    {
        lwp_trace ("Terminate %d", &Public);

        ++ UserCount;

        FD = -1;
        
        if (Element) /* connect handler already called? */
        {
            if (Server.Handlers.Disconnect)
                Server.Handlers.Disconnect (Server.Server, Public);

            Server.Clients.Erase (Element);
        }

        if (SSL)
        {
            if (SSL->Pumping)
                SSL->Dead = true;
            else
                delete SSL;
        }
    }

    int UserCount;
    bool Dead;

    SSLClient * SSL;

    AddressWrapper Address;

    List <Server::Client::Internal *>::Element * Element;

    int FD;
    void * GoneKey;

    static void onSSLHandshook (SSLClient &ssl_client)
    {
        Internal &client = *(Internal *) ssl_client.tag;
        Server::Internal &server = client.Server;

        #ifdef LacewingNPN
            lwp_trace ("onSSLHandshook for %p, NPN is %s",
                            &client, ssl_client.NPN);
        #endif

        assert (!client.Element);

        ++ client.UserCount;

        if (server.Handlers.Connect)
        {
            server.Handlers.Connect
                (server.Server, client.Public);
        }

        if (client.Dead)
        {
            delete &client;
            return;
        }

        -- client.UserCount;
        client.Element = server.Clients.Push (&client);
    }
};

Server::Client::Client (Lacewing::Pump &Pump, int FD) : FDStream (Pump)
{
    SetFD (FD, 0, true);
}

Server::Server (Lacewing::Pump &Pump)
{
    lwp_init ();

    #ifdef LacewingNPN
        lwp_trace ("NPN is available\n");
    #else
        lwp_trace ("NPN is NOT available\n");
    #endif
    
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
            
        lwp_trace ("Trying to accept...");

        if ((FD = accept
            (internal->Socket, (sockaddr *) &Address, &AddressLength)) == -1)
        {
            lwp_trace ("Failed to accept: %s", strerror (errno));
            break;
        }
        
        lwp_trace ("Accepted FD %d", FD);

        Server::Client::Internal * client
                = new (std::nothrow) Server::Client::Internal (*internal, FD);

        if (!client)
        {
            lwp_trace ("Failed allocating client");
            break;
        }

        client->Address.Set (&Address);

        ++ client->UserCount;

        if (!client->SSL)
        {
            if (internal->Handlers.Connect)
                internal->Handlers.Connect (internal->Server, client->Public);
            
            /* Did the client get disconnected by the connect handler? */

            if (client->Dead)
            {
                delete client;
                continue;
            }

            client->Element = internal->Clients.Push (client);
        }
        else
        {
            client->Public.Read (-1);
            
            /* Did the client get disconnected when attempting to read? */

            if (client->Dead)
            {
                delete client;
                continue;
            }
        }

        -- client->UserCount;

        if (internal->Handlers.Receive)
        {
            lwp_trace ("*** READING on behalf of the handler, client tag %p",
                            client->Public.Tag);

            client->Public.AddHandlerData (onClientData, client);
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

        if ((internal->Socket = lwp_create_server_socket
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
    return lwp_socket_port (internal->Socket);
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

#ifdef LacewingNPN

    static int NPN_Advertise (SSL * ssl, const unsigned char ** data,
                            unsigned int * len, void * tag)
    {
        Server::Internal * internal = (Server::Internal *) tag;

        *len = 0;

        for (unsigned char * i = internal->NPN; *i; )
        {
            *len += 1 + *i;
            i += 1 + *i;
        }

        *data = internal->NPN;

        lwp_trace ("Advertising for NPN...");

        return SSL_TLSEXT_ERR_OK;
    }

#endif


bool Server::LoadCertificateFile
        (const char * Filename, const char * Passphrase)
{
    SSL_load_error_strings();

    internal->Context = SSL_CTX_new (SSLv23_server_method ());

    assert (internal->Context);

    strcpy (internal->Passphrase, Passphrase);

    SSL_CTX_set_mode (internal->Context,
        SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER

        #ifdef SSL_MODE_RELEASE_BUFFERS
             | SSL_MODE_RELEASE_BUFFERS
        #endif
    );

    #ifdef LacewingNPN
        SSL_CTX_set_next_protos_advertised_cb
            (internal->Context, NPN_Advertise, internal);
    #endif

    SSL_CTX_set_quiet_shutdown (internal->Context, 1);

    SSL_CTX_set_default_passwd_cb (internal->Context, PasswordCallback);
    SSL_CTX_set_default_passwd_cb_userdata (internal->Context, internal);

    if (SSL_CTX_use_certificate_chain_file (internal->Context, Filename) != 1)
    {
        lwp_trace("Failed to load certificate chain file: %s",
                        ERR_error_string(ERR_get_error(), 0));

        internal->Context = 0;
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file
            (internal->Context, Filename, SSL_FILETYPE_PEM) != 1)
    {
        lwp_trace("Failed to load private key file: %s",
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

bool Server::CanNPN ()
{
    #ifdef LacewingNPN
        return true;
    #endif

    return false;
}

void Server::AddNPN (const char * protocol)
{
    #ifdef LacewingNPN

        size_t length = strlen (protocol);

        if (length > 0xFF)
        {
            lwp_trace ("NPN protocol too long: %s", protocol);
            return;
        }

        unsigned char * end = internal->NPN;

        while (*end)
            end += 1 + *end;

        if ((end + length + 2) > (internal->NPN + sizeof (internal->NPN)))
        {
            lwp_trace ("NPN list would have overflowed adding %s", protocol);
            return;
        }

        *end ++ = ((unsigned char) length);
        memcpy (end, protocol, length + 1);

    #endif
}

const char * Server::Client::NPN ()
{
    #ifndef LacewingNPN
        return "";
    #else

        if (internal->SSL)
            return (const char *) internal->SSL->NPN;

        return "";

    #endif
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

void onClientData (Stream &stream, void * tag, const char * buffer, size_t size)
{
    Server::Client::Internal * client = (Server::Client::Internal *) tag;
    Server::Internal &server = client->Server;

    assert ( (!client->SSL) || client->SSL->Handshook );
    assert (server.Handlers.Receive);

    server.Handlers.Receive (server.Server, client->Public, buffer, size);
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
    internal->Handlers.Receive = onReceive;

    if (onReceive)
    {
        /* Setting onReceive to a handler */

        if (!internal->Handlers.Receive)
        {
            for (List <Server::Client::Internal *>::Element * E
                    = internal->Clients.First; E; E = E->Next)
            {
                (** E)->Public.AddHandlerData (onClientData, (** E));
                (** E)->Public.Read (-1);
            }
        }
        
        return;
    }

    /* Setting onReceive to 0 */

    for (List <Server::Client::Internal *>::Element * E
            = internal->Clients.First; E; E = E->Next)
    {
        (** E)->Public.RemoveHandlerData (onClientData, (** E));
    }
}

AutoHandlerFunctions (Server, Connect)
AutoHandlerFunctions (Server, Disconnect)
AutoHandlerFunctions (Server, Error)

