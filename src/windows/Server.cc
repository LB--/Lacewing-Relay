
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
#include "../Address.h"
#include "WinSSLClient.h"

static void onClientClose (Stream &, void * tag);
static void onClientData (Stream &, void * tag, const char * buffer, size_t size);

struct Server::Internal
{
    Lacewing::Server &Public;

    SOCKET Socket;

    Lacewing::Pump &Pump;
    
    struct
    {
        HandlerConnect Connect;
        HandlerDisconnect Disconnect;
        HandlerReceive Receive;
        HandlerError Error;

    } Handlers;

    Internal (Lacewing::Server &_Public, Lacewing::Pump &_Pump)
                : Public (_Public), Pump (_Pump)
    {
        Socket = -1;
        
        memset (&Handlers, 0, sizeof (Handlers));

        CertificateLoaded = false;

        AcceptsPosted = 0;
    }

    ~ Internal ()
    {
    }
    
    List <Server::Client::Internal *> Clients;

    bool CertificateLoaded;
    CredHandle SSLCreds;

    bool IssueAccept ();
    int AcceptsPosted;
};
    
struct Server::Client::Internal
{
    Lacewing::Server::Client Public;

    Lacewing::Server::Internal &Server;

    Internal (Lacewing::Server::Internal &_Server, HANDLE _FD)
        : Server (_Server), FD (_FD), Public (_Server.Pump, _FD)
    {
        UserCount = 0;
        Dead = false;

        Public.internal = this;
        Public.Tag = 0;

        /* The first added close handler is always the last called.
         * This is important, because ours will destroy the client.
         */

        Public.AddHandlerClose (onClientClose, this);

        if (Server.CertificateLoaded)
        {
            /* TODO : negates the std::nothrow when accepting a client */

            SSL = new WinSSLClient (Server.SSLCreds);
            
            Public.AddFilterDownstream (SSL->Downstream, false, true);
            Public.AddFilterUpstream (SSL->Upstream, false, true);
        }
        else
        {   SSL = 0;
        }
    }

    ~ Internal ()
    {
        lwp_trace ("Terminate %p", &Public);

        ++ UserCount;

        FD = INVALID_HANDLE_VALUE;
        
        if (Element) /* connect handler already called? */
        {
            if (Server.Handlers.Disconnect)
                Server.Handlers.Disconnect (Server.Public, Public);

            Server.Clients.Erase (Element);
        }

        delete SSL;
    }

    int UserCount;
    bool Dead;

    WinSSLClient * SSL;

    AddressWrapper Address;

    List <Server::Client::Internal *>::Element * Element;

    HANDLE FD;
};

Server::Client::Client (Lacewing::Pump &Pump, HANDLE FD) : FDStream (Pump)
{
    SetFD (FD);
}

Server::Server (Lacewing::Pump &Pump)
{
    lwp_init ();
    
    internal = new Server::Internal (*this, Pump);
    Tag = 0;
}

Server::~Server ()
{
    Unhost();

    delete internal;
}

const int IdealPendingAcceptCount = 16;

struct AcceptOverlapped
{
    OVERLAPPED Overlapped;

    HANDLE FD;

    char addr_buffer [(sizeof (sockaddr_storage) + 16) * 2];
};

bool Server::Internal::IssueAccept ()
{
    AcceptOverlapped * overlapped = new (std::nothrow) AcceptOverlapped;
    memset (overlapped, 0, sizeof (AcceptOverlapped));

    if (!overlapped)
        return false;

    if ((overlapped->FD = (HANDLE) WSASocket
            (lwp_socket_addr (Socket).ss_family, SOCK_STREAM, IPPROTO_TCP,
                    0, 0, WSA_FLAG_OVERLAPPED)) == INVALID_HANDLE_VALUE)
    {
        delete overlapped;
        return false;
    }

    lwp_disable_ipv6_only ((lwp_socket) overlapped->FD);

    DWORD bytes_received; 

    /* TODO : Use AcceptEx to receive the first data? */

    if (!AcceptEx (Socket, (SOCKET) overlapped->FD, overlapped->addr_buffer,
        0, sizeof (sockaddr_storage) + 16, sizeof (sockaddr_storage) + 16,
            &bytes_received, (OVERLAPPED *) overlapped))
    {
        int error = WSAGetLastError ();

        if (error != ERROR_IO_PENDING)
            return false;
    }

    ++ AcceptsPosted;

    return true;
}

static void ListenSocketCompletion (void * tag, OVERLAPPED * _overlapped,
                                    unsigned int bytes_transferred, int error)
{
    Server::Internal * internal = (Server::Internal *) tag;
    AcceptOverlapped * overlapped = (AcceptOverlapped *) _overlapped;

    -- internal->AcceptsPosted;

    if (error)
    {
        delete overlapped;
        return;
    }

    while (internal->AcceptsPosted < IdealPendingAcceptCount)
        if (!internal->IssueAccept ())
            break;

    setsockopt ((SOCKET) overlapped->FD, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                    (char *) &internal->Socket, sizeof (internal->Socket));

    sockaddr_storage * local_addr, * remote_addr;
    int local_addr_len, remote_addr_len;

    GetAcceptExSockaddrs
    (
        overlapped->addr_buffer,
        0,

        sizeof (sockaddr_storage) + 16,
        sizeof (sockaddr_storage) + 16,

        (sockaddr **) &local_addr,
        &local_addr_len,

        (sockaddr **) &remote_addr,
        &remote_addr_len
    );

    Server::Client::Internal * client = new (std::nothrow)
            Server::Client::Internal (*internal, overlapped->FD);

    if (!client)
    {
        closesocket ((SOCKET) overlapped->FD);
        delete overlapped;

        return;
    }

    client->Address.Set (remote_addr);
    delete overlapped;

    ++ client->UserCount;

    if (internal->Handlers.Connect)
        internal->Handlers.Connect (internal->Public, client->Public);

    if (client->Dead)
    {
        delete client;
        return;
    }

    client->Element = internal->Clients.Push (client);

    -- client->UserCount;

    if (internal->Handlers.Receive)
    {
        client->Public.AddHandlerData (onClientData, client);
        client->Public.Read (-1);
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
    Unhost ();

    Lacewing::Error Error;

    if ((internal->Socket = lwp_create_server_socket
            (Filter, SOCK_STREAM, IPPROTO_TCP, Error)) == -1)
    {
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    if (listen (internal->Socket, SOMAXCONN) == -1)
    {
        Error.Add (WSAGetLastError ());
        Error.Add ("Error listening");

        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);
        
        return;
    }

    internal->Pump.Add
        ((HANDLE) internal->Socket, internal, ListenSocketCompletion);

    while (internal->AcceptsPosted < IdealPendingAcceptCount)
        if (!internal->IssueAccept ())
            break;
}

void Server::Unhost ()
{
    if(!Hosting ())
        return;

    closesocket (internal->Socket);
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

bool Server::LoadSystemCertificate (const char * StoreName, const char * CommonName, const char * Location)
{
    if (Hosting () || CertificateLoaded ())
    {
        Lacewing::Error Error;
        Error.Add ("Either the server is already hosting, or a certificate has already been loaded");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (internal->Public, Error);

        return false;
    }

    if(!Location || !*Location)
        Location = "CurrentUser";

    if(!StoreName || !*StoreName)
        StoreName = "MY";

    int LocationID = -1;

    do
    {
        if(!strcasecmp(Location, "CurrentService"))
        {
            LocationID = 0x40000; /* CERT_SYSTEM_STORE_CURRENT_SERVICE */
            break;
        }

        if(!strcasecmp(Location, "CurrentUser"))
        {
            LocationID = 0x10000; /* CERT_SYSTEM_STORE_CURRENT_USER */
            break;
        }

        if(!strcasecmp(Location, "CurrentUserGroupPolicy"))
        {
            LocationID = 0x70000; /* CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY */
            break;
        }

        if(!strcasecmp(Location, "LocalMachine"))
        {
            LocationID = 0x20000; /* CERT_SYSTEM_STORE_LOCAL_MACHINE */
            break;
        }

        if(!strcasecmp(Location, "LocalMachineEnterprise"))
        {
            LocationID = 0x90000; /* CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE */
            break;
        }

        if(!strcasecmp(Location, "LocalMachineGroupPolicy"))
        {
            LocationID = 0x80000; /* CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY */
            break;
        }

        if(!strcasecmp(Location, "Services"))
        {
            LocationID = 0x50000; /* CERT_SYSTEM_STORE_SERVICES */
            break;
        }

        if(!strcasecmp(Location, "Users"))
        {
            LocationID = 0x60000; /* CERT_SYSTEM_STORE_USERS */
            break;
        }

    } while(0);
    
    if(LocationID == -1)
    {
        Lacewing::Error Error;
        
        Error.Add("Unknown certificate location: %s", Location);
        Error.Add("Error loading certificate");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (internal->Public, Error);

        return false;
    }

    HCERTSTORE CertStore = CertOpenStore
    (
        (LPCSTR) 9, /* CERT_STORE_PROV_SYSTEM_A */
        0,
        0,
        LocationID,
        StoreName
    );

    if(!CertStore)
    {
        Lacewing::Error Error;

        Error.Add(WSAGetLastError ());
        Error.Add("Error loading certificate");

        if (internal->Handlers.Error)
            internal->Handlers.Error (internal->Public, Error);

        return false;
    }

    PCCERT_CONTEXT Context = CertFindCertificateInStore
    (
        CertStore,
        X509_ASN_ENCODING,
        0,
        CERT_FIND_SUBJECT_STR_A,
        CommonName,
        0
    );

    if(!Context)
    {
        int Code = GetLastError();

        Context = CertFindCertificateInStore
        (
            CertStore,
            PKCS_7_ASN_ENCODING,
            0,
            CERT_FIND_SUBJECT_STR_A,
            CommonName,
            0
        );

        if(!Context)
        {
            Lacewing::Error Error;
                
            Error.Add(Code);
            Error.Add("Error finding certificate in store");

            if (internal->Handlers.Error)
                internal->Handlers.Error (internal->Public, Error);

            return false;
        }
    }

    SCHANNEL_CRED Creds;

    memset(&Creds, 0, sizeof (Creds));

    Creds.dwVersion = SCHANNEL_CRED_VERSION;
    Creds.cCreds = 1;
    Creds.paCred = &Context;
    Creds.grbitEnabledProtocols = (0x80 | 0x40); /* SP_PROT_TLS1 */

    {   TimeStamp ExpiryTime;

        int Result = AcquireCredentialsHandleA
        (
            0,
            (SEC_CHAR *) UNISP_NAME_A,
            SECPKG_CRED_INBOUND,
            0,
            &Creds,
            0,
            0,
            &internal->SSLCreds,
            &ExpiryTime
        );

        if(Result != SEC_E_OK)
        {
            Lacewing::Error Error;
            
            Error.Add(Result);
            Error.Add("Error acquiring credentials handle");

            if (internal->Handlers.Error)
                internal->Handlers.Error (internal->Public, Error);

            return false;
        }

    }

    internal->CertificateLoaded = true;
    return true;
}

bool Server::LoadCertificateFile (const char * Filename, const char * CommonName)
{
    if (!lw_file_exists (Filename))
    {
        Lacewing::Error Error;
        
        Error.Add("File not found: %s", Filename);
        Error.Add("Error loading certificate");

        if (internal->Handlers.Error)
            internal->Handlers.Error (internal->Public, Error);

        return false;
    }

    if (Hosting ())
        Unhost ();

    if(CertificateLoaded())
    {
        FreeCredentialsHandle (&internal->SSLCreds);
        internal->CertificateLoaded = false;
    }

    HCERTSTORE CertStore = CertOpenStore
    (
        (LPCSTR) 7 /* CERT_STORE_PROV_FILENAME_A */,
        X509_ASN_ENCODING,
        0,
        CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG,
        Filename
    );

    bool PKCS7 = false;

    if(!CertStore)
    {
        CertStore = CertOpenStore
        (
            (LPCSTR) 7 /* CERT_STORE_PROV_FILENAME_A */,
            PKCS_7_ASN_ENCODING,
            0,
            CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG,
            Filename
        );

        PKCS7 = true;

        if(!CertStore)
        {
            Lacewing::Error Error;
            
            Error.Add(GetLastError ());
            Error.Add("Error opening certificate file : %s", Filename);

            if (internal->Handlers.Error)
                internal->Handlers.Error (internal->Public, Error);

            return false;
        }
    }

    PCCERT_CONTEXT Context = CertFindCertificateInStore
    (
        CertStore,
        PKCS7 ? PKCS_7_ASN_ENCODING : X509_ASN_ENCODING,
        0,
        CERT_FIND_SUBJECT_STR_A,
        CommonName,
        0
    );

    if(!Context)
    {
        int Code = GetLastError();

        Context = CertFindCertificateInStore
        (
            CertStore,
            PKCS7 ? X509_ASN_ENCODING : PKCS_7_ASN_ENCODING,
            0,
            CERT_FIND_SUBJECT_STR_A,
            CommonName,
            0
        );

        if(!Context)
        {
            Lacewing::Error Error;

            Error.Add(Code);
            Error.Add("Error finding certificate in store");

            if (internal->Handlers.Error)
                internal->Handlers.Error (internal->Public, Error);

            return false;
        }
    }

    SCHANNEL_CRED Creds;

    memset(&Creds, 0, sizeof (Creds));

    Creds.dwVersion = SCHANNEL_CRED_VERSION;
    Creds.cCreds = 1;
    Creds.paCred = &Context;
    Creds.grbitEnabledProtocols = 0xF0; /* SP_PROT_SSL3TLS1 */

    {   TimeStamp ExpiryTime;

        int Result = AcquireCredentialsHandleA
        (
            0,
            (SEC_CHAR *) UNISP_NAME_A,
            SECPKG_CRED_INBOUND,
            0,
            &Creds,
            0,
            0,
            &internal->SSLCreds,
            &ExpiryTime
        );

        if(Result != SEC_E_OK)
        {
            Lacewing::Error Error;
            
            Error.Add(Result);
            Error.Add("Error acquiring credentials handle");

            if (internal->Handlers.Error)
                internal->Handlers.Error (internal->Public, Error);

            return false;
        }

    }

    internal->CertificateLoaded = true;

    return true;
}

bool Server::CertificateLoaded ()
{
    return internal->CertificateLoaded;   
}

bool Server::CanNPN ()
{
    /* NPN is currently not available w/ schannel */

    return false;
}

void Server::AddNPN (const char *)
{
}

const char * Server::Client::NPN ()
{
    return "";
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

