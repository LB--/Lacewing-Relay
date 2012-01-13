
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
#include "../QueuedSend.h"
#include "SendFile.h"

struct Server::Client::Internal
{
    Lacewing::Server::Internal &Server;

    Internal (Lacewing::Server::Internal &_Server)
        : Server (_Server)
    {
        Public.internal = this;
        Public.Tag = 0;

        Context = 0;
        SSLReadWhenWriteReady = false;

        Transfer = 0;
    }

    ~ Internal ()
    {
        if(Context)
            SSL_free(Context);
    }

    Lacewing::Server::Client Public;

    AddressWrapper Address;
    sockaddr_storage SockAddr;

    List <Lacewing::Server::Client::Internal *>::Element * Element;

    int Socket;
    void * GoneKey;
    
    SSL * Context;
    BIO * SSLBio;

    bool SSLReadWhenWriteReady;

    QueuedSendManager QueuedSends;

    bool Send         (QueuedSend * Queued, const char * Data, int Size);
    bool SendFile     (bool AllowQueue, const char * Filename, lw_i64 Offset, lw_i64 Size);
    bool SendWritable (bool AllowQueue, char * Data, int Size);
    
    void DoNextQueued()
    {
        for(;;)
        {
            QueuedSend * First = QueuedSends.First;

            if(First && First->Type == QueuedSendType::Data)
            {
                bool SentImmediately = Send(First, First->Data.Buffer + First->DataOffset,
                                                     First->Data.Size - First->DataOffset);

                if(SentImmediately)
                {
                    QueuedSends.First = First->Next;
                    delete First;

                    if(!QueuedSends.First)
                        QueuedSends.Last = 0;
                }
                else
                {
                    /* The new Send() didn't complete immediately, so we left it in the queue.
                       Relying on the completion of that to send the rest of the queued files/data.  */
                    
                    break;
                }
            }
            
            if(First = QueuedSends.First)
            {
                if (First->Type == QueuedSendType::Disconnect)
                {
                    Terminate ();
                    break;
                }
                else if (First->Type == QueuedSendType::File)
                {
                    bool SentImmediately = SendFile(false, First->Filename, First->FileOffset, First->FileSize);
                    
                    QueuedSends.First = First->Next;
                    delete First;

                    if(!QueuedSends.First)
                        QueuedSends.Last = 0;
                        
                    if(!SentImmediately)
                    {
                        /* The new SendFile() didn't complete immediately, and is now in the queue.
                           Relying on the completion of that to send the rest of the queued files/data. */
                        
                        break;
                    }
                }

                continue;
            }

            /* Nothing left */

            Public.Flush();
            break;
        }
    }
    
    FileTransfer * Transfer;

    void Terminate ();
};

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
        Context =  0;
        
        memset (&Handlers, 0, sizeof (Handlers));

        Nagle = true;

        BytesReceived = 0;
    }

    ~ Internal ()
    {
    }
    
    Backlog <Server::Client::Internal> ClientStructureBacklog;

    List <Server::Client::Internal *> Clients;

    SSL_CTX * Context;

    char Passphrase[128];
    bool Nagle;

    lw_i64 BytesReceived;

    /* When multithreading support is added, there will be one ReceiveBuffer per client.
       Until then, we can save RAM by having a single ReceiveBuffer global to the server. */

    ReceiveBuffer Buffer;
};
    
void Server::Client::Internal::Terminate ()
{
    DebugOut ("Terminate %d", &Public);

    shutdown (Socket, SHUT_RDWR);
    close (Socket);

    Server.Pump.Remove (GoneKey);
    Socket = -1;
    
    if (Server.Handlers.Disconnect)
        Server.Handlers.Disconnect (Server.Server, Public);

    Server.Clients.Erase (Element);
    Server.ClientStructureBacklog.Return (*this);
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

static void ClientSocketReadReady (Server::Client::Internal &Client)
{
    Server::Internal * internal = &Client.Server;
    
    if(Client.Transfer)
    {
        if(!Client.Transfer->ReadReady ())
        {
            delete Client.Transfer;
            Client.Transfer = 0;

            Client.DoNextQueued();
        }
    }

    /* Data is ready to receive */

    int Received;
    bool Disconnected = false;
    
    for(;;)
    {
        internal->Buffer.Prepare ();

        if (!internal->Context)
        {
            /* Normal receive */

            Received = recv (Client.Socket, internal->Buffer, internal->Buffer.Size, MSG_DONTWAIT);
            
            if(Received < 0)
            {
                Disconnected = (errno != EAGAIN && errno != EWOULDBLOCK);
                break;
            }
            
            if (Received == 0)
            {
                Disconnected = true;
                break;
            }
        }
        else
        {
            /* SSL receive - first we need to check if a previous SSL_write gave an SSL_ERROR_WANT_READ */
            
            {   QueuedSend * First = Client.QueuedSends.First;
            
                if(First && First->Type == QueuedSendType::SSLWriteWhenReadable)
                {
                    int Error = SSL_write(Client.Context, First->Data.Buffer, First->Data.Size);
                
                    if(Error == SSL_ERROR_NONE)
                    {
                        Client.DoNextQueued();
                        break;
                    }

                    switch(SSL_get_error(Client.Context, Error))
                    {
                        case SSL_ERROR_WANT_READ:
                        
                            /* Not enough incoming data to do some SSL stuff.  The outgoing data will
                               have to wait until next time the socket is readable. */

                            break;
                        
                        case SSL_ERROR_WANT_WRITE:
                        
                            /* Although we've now satisfied the "readable" condition, the socket isn't
                               actually writable.  Change the queued type to regular data, which will
                               be written next time the socket is writable. */
                            
                            First->Type = QueuedSendType::Data;
                            break;
                            
                        default:
                            
                            Disconnected = true;
                            break;
                    };                    
                }                   
            }
            
            /* Now do the actual read */

            Received = SSL_read(Client.Context, internal->Buffer, internal->Buffer.Size);

            if(Received == 0)
            {
                close (Client.Socket);
                Disconnected = true;

                break;
            }
            
            if(Received < 0)
            {
                int Error = SSL_get_error(Client.Context, Received);
                
                if(Error == SSL_ERROR_WANT_READ)
                {
                    /* We'll call SSL_read again as soon as more data is available
                       anyway, so there's nothing left to do here. */

                    return;
                }

                if(Error == SSL_ERROR_WANT_WRITE)
                {
                    /* SSL_read wants to send some data, but the socket isn't ready
                       for writing at the moment. */

                    Client.SSLReadWhenWriteReady = true;
                    return;
                }

                /* Unknown error */
                
                close (Client.Socket);
                Disconnected = true;

                break;
            }
        }

        internal->Buffer.Received (Received);
        internal->BytesReceived += Received;

        if (internal->Handlers.Receive)
            internal->Handlers.Receive (internal->Server, Client.Public, internal->Buffer, Received);
    }
    
    if (Disconnected)
    {
        Client.Terminate ();
    }
}

void ClientSocketWriteReady (Server::Client::Internal &Client)
{
    if(Client.SSLReadWhenWriteReady)
    {
        Client.SSLReadWhenWriteReady = false;
        ClientSocketReadReady(Client);
    }

    Server::Internal &Internal = Client.Server;

    if(Client.Transfer)
    {
        if(!Client.Transfer->WriteReady())
        {
            delete Client.Transfer;
            Client.Transfer = 0;

            Client.DoNextQueued();
        }
    }
    else
    {
        if(Client.QueuedSends.First && Client.QueuedSends.First->Type == QueuedSendType::Data && !Client.Transfer)
        {
            /* Data has been queued because it couldn't be sent immediately,
               not because of a currently executing file transfer */

            Client.DoNextQueued();
        }
    }
}

static void ListenSocketReadReady (Server::Internal * internal)
{
    sockaddr_storage Address;
    socklen_t AddressLength = sizeof(Address);
    
    for(;;)
    {
        Server::Client::Internal &Client = internal->ClientStructureBacklog.Borrow (*internal);

        {   socklen_t AddressLength = sizeof (Client.SockAddr);
            
            if ((Client.Socket = accept (internal->Socket, (sockaddr *) &Client.SockAddr, &AddressLength)) == -1)
            {
                internal->ClientStructureBacklog.Return (Client);
                break;
            }
        }
        
        fcntl (Client.Socket, F_SETFL, fcntl (Client.Socket, F_GETFL, 0) | O_NONBLOCK);
        
        DisableSigPipe (Client.Socket);

        if (!internal->Nagle)
            DisableNagling (Client.Socket);

        Client.Address.Set (&Client.SockAddr);

        if (internal->Context)
        {
            Client.Context = SSL_new (internal->Context);

            Client.SSLBio = BIO_new_socket(Client.Socket, BIO_NOCLOSE);
            SSL_set_bio(Client.Context, Client.SSLBio, Client.SSLBio);
            
            SSL_set_accept_state(Client.Context);
        }
        
        if (internal->Handlers.Connect)
            internal->Handlers.Connect (internal->Server, Client.Public);
        
        Client.GoneKey = internal->Pump.Add
        (
            Client.Socket,
            &Client,
            
            (Pump::Callback) ClientSocketReadReady,
            (Pump::Callback) ClientSocketWriteReady
        );
        
        Client.Element = internal->Clients.Push (&Client);
    }
}

void Server::Host (int Port, bool ClientSpeaksFirst)
{
    Filter Filter;
    Filter.LocalPort(Port);

    Host(Filter, ClientSpeaksFirst);
}

void Server::Host (Filter &Filter, bool)
{
    Unhost();
    
    {   Lacewing::Error Error;

        if ((internal->Socket = CreateServerSocket (Filter, SOCK_STREAM, IPPROTO_TCP, Error)) == -1)
        {
            if (internal->Handlers.Error)
                internal->Handlers.Error (*this, Error);

            return;
        }
    }

    if ((!CertificateLoaded()) && (!internal->Nagle))
        ::DisableNagling(internal->Socket);

    if (listen (internal->Socket, SOMAXCONN) == -1)
    {
        Error Error;
        
        Error.Add(errno);
        Error.Add("Error listening");

        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);
        
        return;
    }

    internal->Pump.Add (internal->Socket, internal, (Pump::Callback) ListenSocketReadReady);
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

lw_i64 Server::BytesSent ()
{
    return 0; /* TODO */
}

lw_i64 Server::BytesReceived ()
{
    return internal->BytesReceived;
}

void Server::DisableNagling ()
{
    if (internal->Socket != -1)
    {
        Error Error;
        Error.Add("DisableNagling() can only be called when the server is not hosting");

        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    internal->Nagle = false;
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

bool Server::LoadCertificateFile (const char * Filename, const char * Passphrase)
{
    SSL_load_error_strings();

    internal->Context = SSL_CTX_new (SSLv23_server_method ());

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
        DebugOut("Failed to load certificate chain file: %s", ERR_error_string(ERR_get_error(), 0));

        internal->Context = 0;
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file (internal->Context, Filename, SSL_FILETYPE_PEM) != 1)
    {
        DebugOut("Failed to load private key file: %s", ERR_error_string(ERR_get_error(), 0));

        internal->Context = 0;
        return false;
    }

    return true;
}

bool Server::LoadSystemCertificate (const char * StoreName, const char * CommonName, const char * Location)
{
    Error Error;
    Error.Add("System certificates are only supported on Windows");

    if (internal->Handlers.Error)
        internal->Handlers.Error (*this, Error);

    return false;
}

bool Server::Client::Internal::Send (QueuedSend * Queued, const char * Buffer, int Size)
{
    if(Size == -1)
        Size = strlen(Buffer);

    if(!Size)
        return true;
    
    if((Transfer || QueuedSends.First) && !Queued)
    {
        QueuedSends.Add(Buffer, Size);
        return false;
    }

    if(!Context)
    {
        int Sent = send (Socket, Buffer, Size, LacewingNoSignal);

        if(Sent == Size)
        {
            /* Sent immediately */
            return true;
        }

        if(Sent == -1)
        {
            if(errno == EAGAIN)
            {
                /* Can't send now, queue it for later */

                if(!Queued)
                    QueuedSends.Add(Buffer, Size);

                return false;
            }

            return true;
        }

        /* send() didn't fail, but it didn't send everything either */

        if(!Queued)
        {
            QueuedSends.Add(Buffer + Sent, Size - Sent);
            return false;
        }
            
        Queued->DataOffset += Sent;
        return false;
    }
    else
    {
        /* SSL send */

        for(;;)
        {
            int Error = SSL_write(Context, Buffer, Size);

            if(Error > 0)
                return true;

            switch(SSL_get_error(Context, Error))
            {
                case SSL_ERROR_WANT_READ:

                    /* More data from the remote is required before we can write. */
                    
                    if(!Queued)
                        QueuedSends.AddSSLWriteWhenReadable(Buffer, Size);
                    
                    return false;

                case SSL_ERROR_WANT_WRITE:

                    /* The socket isn't ready for writing to right now. */
                    
                    if(!Queued)
                        QueuedSends.Add(Buffer, Size);
                    
                    return false;

                default:

                    /* Unknown error */
                    return true;
            };
            
            break;
        }
    }

    return true;
}

void Server::Client::Send (const char * Buffer, int Size)
{
    internal->Send (0, Buffer, Size);
}

void Server::Client::SendWritable (char * Buffer, int Size)
{
    /* This only differs for Windows */

    internal->Send (0, Buffer, Size);
}

bool Server::Client::Internal::SendFile (bool AllowQueue, const char * Filename, lw_i64 Offset, lw_i64 Size)
{   
    if(AllowQueue && (QueuedSends.First || Transfer))
    {        
        QueuedSends.Add(Filename, Offset, Size);
        return false;
    }
    
    int File = open(Filename, O_RDONLY, 0);

    if(File == -1)
    {
        DebugOut("Failed to open %s", Filename);

        Public.Flush();
        return true;
    }
    
    if(!Size)
    {
        struct stat FileStat;
        
        if(fstat(File, &FileStat) == -1)
        {
            DebugOut("Failed to stat %s", Filename);

            close(File);
            Public.Flush();

            return true;
        }
        
        Size = FileStat.st_size;
    }
    
    if(Context)
    {
        /* SSL - have to send the file manually */

        lseek(File, Offset, SEEK_SET);
        
        Transfer = new SSLFileTransfer(File, Context, Size);

        if(!Transfer->WriteReady ())
        {
            /* Either completed immediately or failed */

            DebugOut("SSL file transfer completed immediately or failed");

            delete Transfer;
            Transfer = 0;
            
            Public.Flush();
            return true;
        }

        return false;
    }
    else
    {
        /* Non SSL - can use kernel sendfile.  First try to send whatever we can of the file with
           LacewingSendFile, and then if that doesn't fail or complete immediately, create a RawFileTransfer. */
    
        off_t _Offset = Offset;
        off_t _Size   = Size;

        if((!LacewingSendFile(File, Socket, _Offset, _Size)) || _Size == 0)
        {
            /* Failed or whole file sent immediately */
            
            close(File);        
            Public.Flush();

            return true;
        }

        Transfer = new RawFileTransfer(File, Socket, _Offset, _Size);
        return false;
    }
    
    return true;
}

void Server::Client::SendFile (const char * Filename, lw_i64 Offset, lw_i64 Size)
{
    internal->SendFile (true, Filename, Offset, Size);
}

bool Server::Client::CheapBuffering ()
{
    return true;
}

void Server::Client::StartBuffering ()
{
    if (internal->Context)
        return;

    #ifdef LacewingAllowCork
        int Enabled = 1;
        setsockopt (internal->Socket, IPPROTO_TCP, LacewingCork, &Enabled, sizeof (Enabled));
    #endif
}

void Server::Client::Flush ()
{
    if (internal->Context)
        return;

    #ifdef LacewingAllowCork
        int Enabled = 0;
        setsockopt (internal->Socket, IPPROTO_TCP, LacewingCork, &Enabled, sizeof (Enabled));
    #endif
}

void Server::Client::Disconnect ()
{
    /* TODO : Is it safe to remove the client immediately in multithreaded mode? */

    if (internal->QueuedSends.First || internal->Transfer)
    {
        internal->QueuedSends.Add (new QueuedSend (QueuedSendType::Disconnect));
        return;
    }
    else
    {
        if (internal->Context)
            SSL_shutdown (internal->Context);
        else
            shutdown (internal->Socket, SHUT_RD);
    }
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

AutoHandlerFunctions (Server, Connect)
AutoHandlerFunctions (Server, Disconnect)
AutoHandlerFunctions (Server, Receive)
AutoHandlerFunctions (Server, Error)

