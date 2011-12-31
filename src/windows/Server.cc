
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011 James McLaughlin.  All rights reserved.
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

const int IdealPendingAcceptCount = 16;

namespace OverlappedType
{
    enum Type
    {
        FileSend,
        Send,
        Receive,
        ManualFileSendRead,
        Disconnect
    };
}

struct ServerOverlapped
{
    OVERLAPPED Overlapped;

    OverlappedType::Type Type;
    void * Tag;

    ServerOverlapped (Server::Internal &)
    {
        memset(&Overlapped, 0, sizeof(OVERLAPPED));
    }
};

struct SecureClientInternal
{
    Server::Client::Internal &Client;

    SecureClientInternal (Server::Client::Internal &_Client) : Client (_Client)
    {
        Status = SEC_I_CONTINUE_NEEDED;

        HandshakeComplete = false;
        GotContext = false;
    }

    SecPkgContext_StreamSizes Sizes;

    SECURITY_STATUS Status;
    bool HandshakeComplete;

    bool GotContext;
    CtxtHandle Context;

    void ProcessHandshakeData(char * Buffer, unsigned int &Size);
    void ProcessMessageData(char * &Buffer, unsigned int &Size, char * &Extra, unsigned int &ExtraSize);
    
    MessageBuilder Buffer;
};

struct ManualFileSendInformation
{
    Server::Client::Internal &Client;
    HANDLE File;

    char * Buffer;
    size_t BufferSize;

    __int64 Offset;
    __int64 Size;

    ManualFileSendInformation (Server::Client::Internal &_Client)
        : Client (_Client)
    {
    }

    inline int NextRead ()
    {
        return Size < BufferSize ? (int) Size : BufferSize;
    }
};

struct Server::Client::Internal
{
    /* Only one receive ever takes place on the same client at a time, so it's safe
       to just have one overlapped structure and reuse it */

    ServerOverlapped ReceiveOverlapped;

    void * RemoveKey;

    SecureClientInternal * Secure;

    Lacewing::Server::Client Public;

    SOCKET Socket;

    __int64 BytesSent, BytesReceived;

    bool Accepted, Connecting, Disconnecting;

    WSABUF Buffer;

    AddressWrapper Address;

    Lacewing::Server::Internal &Server;

    /* TODO : Replace with a ReceiveBuffer object */
    char ReceiveBuffer[1024 * 32];

    MessageBuilder OutputBuffer;
    bool BufferingOutput;

    Internal (Lacewing::Server::Internal &);
    ~ Internal ();

    bool SendingFile;
    Sync Sync_SendingFile;

    QueuedSendManager QueuedSends;
    
    void PostReceive();

    void EndFileSend(ServerOverlapped &Overlapped, ManualFileSendInformation &Send);
    
    bool Send          (bool AllowQueue, const char * Data, int Size);
    bool SendFile      (bool AllowQueue, const char * Filename, __int64 Offset, __int64 Size);
    bool SendWritable  (bool AllowQueue, char * Data, int Size);

    List <Server::Client::Internal *>::Element * Element;
};

void Server::Client::Internal::PostReceive ()
{
    memset(&ReceiveOverlapped, 0, sizeof (ServerOverlapped));

    ReceiveOverlapped.Tag = this;
    ReceiveOverlapped.Type = OverlappedType::Receive;

    DWORD Flags = 0;

    if (WSARecv (Socket, &Buffer, 1, 0, &Flags, (OVERLAPPED *) &ReceiveOverlapped, 0) == -1)
    {
        int Error = WSAGetLastError();

        if(Error == WSA_IO_PENDING)
            return;

        Public.Disconnect();
    }
}

struct SendInformation
{
    Server::Client::Internal &Client;
    
    SendInformation (Server::Client::Internal &_Client) : Client (_Client)
    {
    }
};

struct SendFileInformation
{
    Server::Client::Internal &Client;
    HANDLE File;

    SendFileInformation (Server::Client::Internal &_Client) : Client (_Client)
    {
    }
};

struct Server::Internal
{
    Lacewing::Pump &Pump;

    __int64 BytesSent, BytesReceived;

    Lacewing::Server &Public;

    SOCKET Socket;

    List <Server::Client::Internal *> Clients;

    Backlog <Server::Client::Internal>   ClientStructureBacklog;
    Backlog <ServerOverlapped>           OverlappedBacklog;
    Backlog <SendInformation>            SendInformationBacklog;
    Backlog <SendFileInformation>        SendFileInformationBacklog;
    Backlog <ManualFileSendInformation>  ManualFileSendInformationBacklog;
    
    bool PostAccept();
    volatile long AcceptsPosted;

    bool ClientSpeaksFirst;
    bool Nagle;

    bool CertificateLoaded;
    CredHandle ServerCreds;

    Internal (Server &_Public, Lacewing::Pump &_Pump)
            : Public (_Public), Pump (_Pump)
    {
        memset (&Handlers, 0, sizeof (Handlers));

        AcceptsPosted = 0;

        Socket = -1;

        BytesSent = 0;
        BytesReceived = 0;

        CertificateLoaded = false;
        ClientSpeaksFirst = false;

        Nagle = true;
    }

    struct
    {
        HandlerConnect    Connect;
        HandlerDisconnect Disconnect;
        HandlerReceive    Receive;
        HandlerError      Error;

    } Handlers;

    bool Tick(bool Block);
};

Server::Client::Internal::Internal (Server::Internal &_Server)
    : Server (_Server), ReceiveOverlapped (_Server)
{
    RemoveKey = 0;

    Public.internal = this;
    Public.Tag = 0;

    Buffer.len = sizeof(ReceiveBuffer);
    Buffer.buf = ReceiveBuffer;

    memset (&ReceiveOverlapped, 0, sizeof (OVERLAPPED));

    DisableIPV6Only
        (Socket = socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP));

    Accepted = false;

    if(!Server.Nagle)
        ::DisableNagling(Socket);

    Element = 0;

    Disconnecting   = false;
    SendingFile     = false;
    BufferingOutput = false;

    Secure = Server.Public.CertificateLoaded() ? new SecureClientInternal(*this) : 0;
}

Server::Client::Internal::~ Internal ()
{
    LacewingCloseSocket(Socket);

    delete Secure;

    if (RemoveKey)
        Server.Pump.Remove (RemoveKey);
}

bool Server::Internal::PostAccept ()
{
    Server::Client::Internal &Client = ClientStructureBacklog.Borrow (*this);

    Client.ReceiveOverlapped.Tag = &Client;

    DWORD BytesReceived; /* Not used, but MSDN doesn't say the parameter can be NULL */

    if (!AcceptEx (Socket, Client.Socket, Client.Buffer.buf,
        Client.Server.ClientSpeaksFirst ? (Client.Buffer.len - ((sizeof (sockaddr_storage) + 16) * 2)) : 0,
        sizeof (sockaddr_storage) + 16, sizeof (sockaddr_storage) + 16, &BytesReceived, (OVERLAPPED *) &Client.ReceiveOverlapped))
    {
        int Error = WSAGetLastError();

        if(Error != ERROR_IO_PENDING)
        {
            ClientStructureBacklog.Return(Client);
            return false;
        }
    }

    InterlockedIncrement(&AcceptsPosted);
    return true;
}

void Server::Client::Internal::EndFileSend (ServerOverlapped &Overlapped, ManualFileSendInformation &Send)
{
    Sync::Lock Lock (Send.Client.Sync_SendingFile);

    delete [] Send.Buffer;
    CloseHandle(Send.File);

    Server.OverlappedBacklog.Return(Overlapped);

    for(;;)
    {
        /* If there's data to be sent after the file, send it */

        if(QueuedSends.First && QueuedSends.First->Type == QueuedSendType::Data)
        {
            this->Send(false, QueuedSends.First->Data.Buffer, QueuedSends.First->Data.Size);

            QueuedSend * ToDelete = QueuedSends.First;
            QueuedSends.First = QueuedSends.First->Next;
            delete ToDelete;

            if(!QueuedSends.First)
                QueuedSends.Last = 0;
        }

        /* If there's another file now, set that away to send */

        if(QueuedSends.First && QueuedSends.First->Type == QueuedSendType::File)
        {
            /* If SendFile returns true, we just set off another manual file send, which
                    will send the next after file data later */

            bool Finished = SendFile(false, QueuedSends.First->Filename, QueuedSends.First->FileOffset, QueuedSends.First->FileSize);

            QueuedSend * ToDelete = QueuedSends.First;
            QueuedSends.First = QueuedSends.First->Next;
            delete ToDelete;

            if(!QueuedSends.First)
                QueuedSends.Last = 0;

            if(Finished)
                break;
        }
        else
        {
            /* No more files */

            Send.Client.SendingFile = false;
            break;
        }
    }
    
    Server.ManualFileSendInformationBacklog.Return(Send);
}

static void Disconnecter (Server::Client::Internal &Client)
{
    Server::Internal &Server = Client.Server;

    LacewingCloseSocket (Client.Socket);
    Client.Socket = -1;

    Server.Clients.Erase (Client.Element);
    
    if (Server.Handlers.Disconnect)
        Server.Handlers.Disconnect (Server.Public, Client.Public);

    if(Client.Secure)
    {
        DeleteSecurityContext(&Client.Secure->Context);
    }

    Server.ClientStructureBacklog.Return(Client);
}

void Server::Client::Disconnect ()
{
    if (internal->Disconnecting)
        return;

    internal->Disconnecting = true;

    shutdown (internal->Socket, SD_RECEIVE);
    
    /* Don't post anything if the connect is still being processed - the thread that called
       the connect handler will go on to handle the disconnect if Disconnecting = true */

    if (internal->Connecting)
        return;

    internal->Server.Pump.Post ((void *) Disconnecter, internal);
}

static void ClientSocketCompletion (Server::Client::Internal &Client, ServerOverlapped &Overlapped, unsigned int BytesTransferred, int Error)
{
    Server::Internal * internal = &Client.Server;

    if(Overlapped.Type == OverlappedType::Send)
    {
        SendInformation &Send = *(SendInformation *) Overlapped.Tag;

        internal->OverlappedBacklog.Return (Overlapped);
        internal->SendInformationBacklog.Return (Send);

        return;
    }

    if(Overlapped.Type == OverlappedType::FileSend)
    {
        SendFileInformation &Send = *(SendFileInformation *) Overlapped.Tag;

        CloseHandle(Send.File);

        internal->OverlappedBacklog.Return (Overlapped);
        internal->SendFileInformationBacklog.Return (Send);

        return;
    }

    if(Overlapped.Type == OverlappedType::Receive)
    {
        if(Client.Disconnecting)
            return;

        if(Error || !BytesTransferred)
        {
            Client.Public.Disconnect();
            return;
        }

        internal->BytesReceived += BytesTransferred;

        char * ReceivedBuffer = Client.Buffer.buf;

        char * SecureExtra;
        unsigned int SecureExtraSize;

    Reprocess:

        if(Client.Secure)
        {
            if(!Client.Secure->HandshakeComplete)
            {
                /* Modifies the contents of the buffer and BytesTransferred
                   to describe anything that's left after the handshake */

                Client.Secure->ProcessHandshakeData(ReceivedBuffer, BytesTransferred);

                if((!Client.Secure->HandshakeComplete) || !BytesTransferred)
                    goto NextReceive;
            }

            /* Modifies what the buffer points to and BytesTransferred
               to describe the decrypted message when it's ready */

            Client.Secure->ProcessMessageData(ReceivedBuffer, BytesTransferred, SecureExtra, SecureExtraSize);

            if(!BytesTransferred)
                goto NextReceive;
        }

        /* Call the receive handler */

        ReceivedBuffer[BytesTransferred] = 0;

        if (internal->Handlers.Receive)
            internal->Handlers.Receive
                (internal->Public, Client.Public, ReceivedBuffer, BytesTransferred);

        /* Move any extra data back to the beginning of the secure buffer, and process it again.
           BytesTransferred == 0 means that ProcessMessageData won't access ReceivedBuffer, but
           it will still use it as an output to store anything it manages to decrypt. */

        if(Client.Secure && SecureExtra)
        {
            memmove(Client.Secure->Buffer.Buffer, SecureExtra, SecureExtraSize);
            Client.Secure->Buffer.Size = SecureExtraSize;
            
            BytesTransferred = 0;

            goto Reprocess;
        }

    NextReceive:

        if(Client.Disconnecting)
        {
            /* Probably set by the receive handler.  No point in posting another receive now. */

            return;
        }

        Client.PostReceive();
        return;
    }
}

void ListenSocketCompletion (Server::Internal * internal, ServerOverlapped &Overlapped, int BytesTransferred, int ErrorCode)
{
    Server::Client::Internal &Client = *(Server::Client::Internal *) Overlapped.Tag;

    InterlockedDecrement (&internal->AcceptsPosted);

    if(ErrorCode)
    {
        internal->ClientStructureBacklog.Return (Client);

        if (internal->Socket == -1)
        {
            /* AcceptEx failed because we stopped hosting */
            return;
        }

        Lacewing::Error Error;

        Error.Add (ErrorCode);
        Error.Add ("Error accepting");

        if (internal->Handlers.Error)
            internal->Handlers.Error (internal->Public, Error);

        return;
    }

    while (internal->AcceptsPosted < IdealPendingAcceptCount)
        if (!internal->PostAccept ())
            break;

    Client.Accepted = true;

    sockaddr_storage * LocalAddress, * RemoteAddress;
    int LocalAddressLength, RemoteAddressLength;

    GetAcceptExSockaddrs
    (
        Client.Buffer.buf,
        
        Client.Server.ClientSpeaksFirst ?
            (Client.Buffer.len - ((sizeof (sockaddr_storage) + 16) * 2)) : 0,

         sizeof (sockaddr_storage) + 16,
         sizeof (sockaddr_storage) + 16,

         (sockaddr **) &LocalAddress,
         &LocalAddressLength,

         (sockaddr **) &RemoteAddress,
         &RemoteAddressLength
    );

    Client.Address.Set (RemoteAddress);

    setsockopt (Client.Socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                    (char *) &internal->Socket, sizeof (internal->Socket));

    if (internal->Handlers.Connect)
        internal->Handlers.Connect (internal->Public, Client.Public);

    /* Connect handler might have called Disconnect() */

    if(!Client.Disconnecting)
    {                    
        Client.ReceiveOverlapped.Type = OverlappedType::Receive;
        Client.Connecting = false;

        Client.Element = internal->Clients.Push (&Client);

        Client.RemoveKey = internal->Pump.Add
            ((HANDLE) Client.Socket, (void *) &Client, (Pump::Callback) ClientSocketCompletion);

        if(BytesTransferred > 0)
        {
            /* Fake a WSARecv completion to call the receive handler for what AcceptEx gave us.
               It will also post the first real WSARecv call. */

            ClientSocketCompletion(Client, Client.ReceiveOverlapped, BytesTransferred, 0);
        }
        else
        {
            /* No bytes were received by AcceptEx, so we're not going to fake a WSARecv completion.
               That means the first WSARecv must be posted here. */

            Client.PostReceive();
        }
    }
    else
    {
        /* The handler disconnected the client.  Fake a Disconnect() completion. */

        Disconnecter(Client);
    }
}

void ManualFileSendReadCompletion (Server::Client::Internal &Client, ServerOverlapped &Overlapped, int BytesTransferred, int Error)
{
    ManualFileSendInformation &Send = *(ManualFileSendInformation *) Overlapped.Tag;

    if(!BytesTransferred)
    {
        /* EOF */
    
        Send.Client.EndFileSend(Overlapped, Send);
        
        return;
    }

    /* Send the data we just read, making sure it's not added as after file data by
       specifying AllowQueue = false */

    Send.Client.SendWritable(false, Send.Buffer, BytesTransferred);
   
    /* Move forward in the file */

    Send.Offset += BytesTransferred;
    Send.Size -= BytesTransferred;

    memset(&Overlapped.Overlapped, 0, sizeof (OVERLAPPED));

    {   LARGE_INTEGER Offset;

        Offset.QuadPart = Send.Offset;

        Overlapped.Overlapped.Offset = Offset.LowPart;
        Overlapped.Overlapped.OffsetHigh = Offset.HighPart;
    }

    int NextRead = Send.NextRead ();

    if (!NextRead)
    {
        /* End of the desired send size */
    
        Send.Client.EndFileSend(Overlapped, Send);
        return;
    }

    /* Read the next file chunk */

    if(!ReadFile(Send.File, Send.Buffer, NextRead, 0, (OVERLAPPED *) &Overlapped))
    {
        int Error = GetLastError();

        if(Error == ERROR_IO_PENDING)
            return;

        Send.Client.EndFileSend(Overlapped, Send);
    }
}

Server::Server (Lacewing::Pump &Pump)
{
    LacewingInitialise();
    internal = new Server::Internal (*this, Pump);    
}

Server::~Server ()
{
    delete internal;
}

void Server::Host (int Port, bool ClientSpeaksFirst)
{
    Filter Filter;
    Filter.LocalPort (Port);

    Host(Filter, ClientSpeaksFirst);
}

void Server::Host (Filter &Filter, bool ClientSpeaksFirst)
{
    Unhost();

    internal->ClientSpeaksFirst = ClientSpeaksFirst;
    
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
        Lacewing::Error Error;
        
        Error.Add(WSAGetLastError ());
        Error.Add("Error listening");

        LacewingCloseSocket (internal->Socket);
        internal->Socket = -1;

        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    internal->Pump.Add
        ((HANDLE) internal->Socket, internal, (Pump::Callback) ListenSocketCompletion);

    while (internal->AcceptsPosted < IdealPendingAcceptCount)
        if (!internal->PostAccept ())
            break;
}

void Server::Unhost ()
{
    if(!Hosting())
        return;

    for (List <Server::Client::Internal *>::Element * E = internal->Clients.First;
            E; E = E->Next)
    {
        (** E)->Public.Disconnect ();
    }

    LacewingCloseSocket (internal->Socket);
    internal->Socket = -1;
}

bool Server::Client::Internal::SendWritable (bool AllowQueue, char * Data, int Size)
{
    if (!Secure || !Secure->HandshakeComplete)
        return Send (AllowQueue, Data, Size);

    if(!Size)
        return true;

    {   int SizeMax = Secure->Sizes.cbMaximumMessage - 8;

        while(Size > SizeMax)
        {
            SendWritable(AllowQueue, Data, SizeMax);
            
            Data += SizeMax;
            Size -= SizeMax;
        }
    }
            
    Sync::Lock Lock (Sync_SendingFile);
    
    if(AllowQueue)
    {
        if(SendingFile)
        {
            QueuedSends.Add(Data, Size);
            return true;
        }
    }

    SecBuffer SendBuffers[4];

        SendBuffers[0].pvBuffer = (char *) alloca(Secure->Sizes.cbHeader);
        SendBuffers[0].cbBuffer = Secure->Sizes.cbHeader;
        SendBuffers[0].BufferType = SECBUFFER_STREAM_HEADER;

        SendBuffers[1].pvBuffer = Data;
        SendBuffers[1].cbBuffer = Size;
        SendBuffers[1].BufferType = SECBUFFER_DATA;

        SendBuffers[2].pvBuffer = (char *) alloca(Secure->Sizes.cbTrailer);
        SendBuffers[2].cbBuffer = Secure->Sizes.cbTrailer;
        SendBuffers[2].BufferType = SECBUFFER_STREAM_TRAILER;

        SendBuffers[3].BufferType = SECBUFFER_EMPTY;
        SendBuffers[3].cbBuffer = 0;

    SecBufferDesc SendBuffersDesc;

        SendBuffersDesc.cBuffers = 4;
        SendBuffersDesc.pBuffers = SendBuffers;
        SendBuffersDesc.ulVersion = SECBUFFER_VERSION;

    SECURITY_STATUS Status = EncryptMessage(&Secure->Context, 0, &SendBuffersDesc, 0);

    if(Status != SEC_E_OK)
        return false;

    WSABUF WSABuffers[4];

    WSABuffers[0].len = SendBuffers[0].cbBuffer;
    WSABuffers[0].buf = (char *) SendBuffers[0].pvBuffer;

    WSABuffers[1].len = SendBuffers[1].cbBuffer;
    WSABuffers[1].buf = (char *) SendBuffers[1].pvBuffer;

    WSABuffers[2].len = SendBuffers[2].cbBuffer;
    WSABuffers[2].buf = (char *) SendBuffers[2].pvBuffer;

    WSABuffers[3].len = SendBuffers[3].cbBuffer;
    WSABuffers[3].buf = (char *) SendBuffers[3].pvBuffer;

    ServerOverlapped &Overlapped = Server.OverlappedBacklog.Borrow(Server);

    Overlapped.Type = OverlappedType::Send;
    Overlapped.Tag  = &Server.SendInformationBacklog.Borrow(*this);

    if(WSASend (Socket, WSABuffers, 4, 0, 0, (OVERLAPPED *) &Overlapped, 0) == -1)
    {
        int Error = WSAGetLastError();

        if(Error != WSA_IO_PENDING)
            return false;
    }

    return true;
}

void Server::Client::SendWritable (char * Data, int Size)
{
    ((Server::Client::Internal *) internal)->SendWritable (true, Data, Size);
}

bool Server::Client::Internal::Send (bool AllowQueue, const char * Data, int Size)
{
    if(Size == -1)
        Size = strlen(Data);
    
    if(!Size)
        return true;
        
    Sync::Lock Lock (Sync_SendingFile);
    
    if(AllowQueue)
    {
        if(SendingFile)
        {
            QueuedSends.Add(Data, Size);
            return true;
        }
    }
    
    if((!Secure) || !Secure->HandshakeComplete)
    {
        /* Unsecure client, or a send as part of the secure handshake */

        WSABUF Buffer = { Size, (CHAR *) Data };

        ServerOverlapped &Overlapped = Server.OverlappedBacklog.Borrow(Server);

        Overlapped.Type = OverlappedType::Send;
        Overlapped.Tag = &Server.SendInformationBacklog.Borrow(*this);

        if (WSASend (Socket, &Buffer, 1, 0, 0, (OVERLAPPED *) &Overlapped, 0) == -1)
        {
            int Error = WSAGetLastError();

            if(Error != WSA_IO_PENDING)
                return false;
        }

        return true;
    }
    else
    {
        /* Secure send */

        char * Copy = new char[Size];
        memcpy(Copy, Data, Size);

        bool Success = SendWritable(AllowQueue, Copy, Size);

        delete [] Copy;

        return Success;
    }
   
    return true;
}

void Server::Client::Send (const char * Data, int Size)
{
    ((Server::Client::Internal *) internal)->Send (true, Data, Size);
}


/* False means the next queued data won't be sent, either because the transfer failed or we were able to use TransmitFile */

bool Server::Client::Internal::SendFile (bool AllowQueue, const char * Filename, __int64 Offset, __int64 Size)
{   
    if (Size == 0)
        return false;

    Sync::Lock Lock (Sync_SendingFile);
    
    if(AllowQueue)
    {
        if(SendingFile)
        {
            QueuedSends.Add(Filename, Offset, Size);
            return true;
        }
    }
    
    HANDLE File = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0,
        OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if(File == INVALID_HANDLE_VALUE)
        return false;

    if (Size == -1)
    {
        LARGE_INTEGER FileSize;

        if (!Compat::GetFileSizeEx () (File, &FileSize))
        {
            CloseHandle(File);
            return false;
        }

        Size = FileSize.QuadPart;
    }

    if((!Secure) && Size < ((((__int64) 1024) * 1024 * 1024 * 2) - 16))
    {
        /* Not secure, and the file is smaller than 2 GB - we can use TransmitFile */

        ServerOverlapped &Overlapped = Server.OverlappedBacklog.Borrow(Server);

        Overlapped.Type = OverlappedType::FileSend;
        Overlapped.Tag = &Server.SendFileInformationBacklog.Borrow(*this);

        ((SendFileInformation *) Overlapped.Tag)->File = File;

        LARGE_INTEGER _Offset;
        _Offset.QuadPart = Offset;

        Overlapped.Overlapped.Offset = _Offset.LowPart;
        Overlapped.Overlapped.OffsetHigh = _Offset.HighPart;

        if(OutputBuffer.Size)
        {
            /* SendFile always flushes when buffering.  If we're able to use TransmitFile,
               we can take advantage of the TRANSMIT_FILE_BUFFERS stuff to send the buffered
               data (HTTP headers, for instance) and the file all in one call. */

            TRANSMIT_FILE_BUFFERS Extra;

            Extra.Head       = OutputBuffer.Buffer;
            Extra.HeadLength = OutputBuffer.Size;

            Extra.Tail       = 0;
            Extra.TailLength = 0;

            if(!TransmitFile(Socket, File, (DWORD) Size, 0, (OVERLAPPED *) &Overlapped, &Extra, 0))
            {
                OutputBuffer.Reset();

                int Error = WSAGetLastError();

                if(Error != WSA_IO_PENDING)
                {
                    CloseHandle(File);
                    return false;
                }
            }
                
            OutputBuffer.Reset();
            return false;
        }

        if(!TransmitFile(Socket, File, (DWORD) Size, 0, (OVERLAPPED *) &Overlapped, 0, 0))
        {
            int Error = WSAGetLastError();

            if(Error != WSA_IO_PENDING)
            {
                CloseHandle(File);
                return false;
            }
        }
        
        return false;
    }

    /* Either secure or the file was bigger than 2 GB - have to send manually */
    
    Public.Flush();
    SendingFile = true;

    /* Add the file handle to the event pump */

    Server.Pump.Add (File, (void *) this, (Pump::Callback) ManualFileSendReadCompletion);

    ManualFileSendInformation &Send = Server.ManualFileSendInformationBacklog.Borrow(*this);
    ServerOverlapped &Overlapped = Server.OverlappedBacklog.Borrow(Server);

    Overlapped.Type = OverlappedType::ManualFileSendRead;
    Overlapped.Tag = &Send;
    
    Send.File   = File;
    Send.Offset = Offset;
    Send.Size   = Size;

    Send.BufferSize = 1024 * 256;

    if(Secure && Secure->Sizes.cbMaximumMessage < Send.BufferSize)
        Send.BufferSize = Secure->Sizes.cbMaximumMessage;

    Send.Buffer = new char[Send.BufferSize];

    if(!ReadFile (File, Send.Buffer, Send.NextRead (), 0, (OVERLAPPED *) &Overlapped))
    {
        int Error = GetLastError();
        
        if(Error != ERROR_IO_PENDING)
        {
            CloseHandle(File);
            return false;
        }
    }

    return true;
}

void Server::Client::SendFile (const char * Filename, __int64 Offset, __int64 Size)
{
    ((Server::Client::Internal *) internal)->SendFile (true, Filename, Offset, Size);
}

bool Server::Client::CheapBuffering ()
{
    /* No TCP_CORK or TCP_NOPUSH on Windows */

    return false;
}

void Server::Client::StartBuffering ()
{
    if (internal->BufferingOutput)
        return;

    internal->BufferingOutput = true;
}

void Server::Client::Flush ()
{
    if (!internal->BufferingOutput)
        return;

    internal->BufferingOutput = false;
    Send (internal->OutputBuffer.Buffer, internal->OutputBuffer.Size);

    internal->OutputBuffer.Reset ();
}

Address &Server::Client::GetAddress ()
{
    return internal->Address;
}

bool Server::Hosting ()
{
    return internal->Socket != -1;
}

int Server::Port ()
{
    return GetSocketPort (internal->Socket);
}

bool Server::LoadSystemCertificate (const char * StoreName, const char * CommonName, const char * Location)
{
    if (Hosting () || CertificateLoaded ())
    {
        Lacewing::Error Error;
        Error.Add("Either the server is already hosting, or a certificate has already been loaded");
        
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

    HCERTSTORE CertStore = CertOpenStore ((LPCSTR) 9, /* CERT_STORE_PROV_SYSTEM_A */
                                0, 0, LocationID, StoreName);

    if(!CertStore)
    {
        Lacewing::Error Error;

        Error.Add(LacewingGetLastError ());
        Error.Add("Error loading certificate");

        if (internal->Handlers.Error)
            internal->Handlers.Error (internal->Public, Error);

        return false;
    }

    PCCERT_CONTEXT Context = CertFindCertificateInStore(CertStore, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR_A, CommonName, 0);

    if(!Context)
    {
        int Code = GetLastError();

        Context = CertFindCertificateInStore(CertStore, PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR_A, CommonName, 0);

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

        int Result = AcquireCredentialsHandleA (0, (SEC_CHAR *) UNISP_NAME_A, SECPKG_CRED_INBOUND, 0, &Creds, 0, 0,
                                                    &internal->ServerCreds, &ExpiryTime);

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
    if (!FileExists (Filename))
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
        FreeCredentialsHandle (&internal->ServerCreds);
        internal->CertificateLoaded = false;
    }

    HCERTSTORE CertStore = CertOpenStore ((LPCSTR) 7 /* CERT_STORE_PROV_FILENAME_A */, X509_ASN_ENCODING, 0,
                                CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG, Filename);

    bool PKCS7 = false;

    if(!CertStore)
    {
        CertStore = CertOpenStore((LPCSTR) 7 /* CERT_STORE_PROV_FILENAME_A */, PKCS_7_ASN_ENCODING, 0,
                                     CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG, Filename);

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

    PCCERT_CONTEXT Context = CertFindCertificateInStore(CertStore, PKCS7 ? PKCS_7_ASN_ENCODING : X509_ASN_ENCODING, 0,
                                                            CERT_FIND_SUBJECT_STR_A, CommonName, 0);

    if(!Context)
    {
        int Code = GetLastError();

        Context = CertFindCertificateInStore(CertStore, PKCS7 ? X509_ASN_ENCODING : PKCS_7_ASN_ENCODING, 0,
                                                CERT_FIND_SUBJECT_STR_A, CommonName, 0);

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

        int Result = AcquireCredentialsHandleA (0, (SEC_CHAR *) UNISP_NAME_A, SECPKG_CRED_INBOUND, 0, &Creds,
                                                   0, 0, &internal->ServerCreds, &ExpiryTime);

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

void SecureClientInternal::ProcessMessageData (char * &Buffer, unsigned int &Size, char * &Extra, unsigned int &ExtraSize)
{
    /* TODO : Currently copying everything before trying DecryptMessage.  Packets certainly aren't
        *always* fragmented, so it might be best to try a DecryptMessage on the buffer first? */

    if(Size > 0)
    {
        this->Buffer.Add(Buffer, Size);
        Size = 0;
    }

    Extra = 0;

    SecBuffer Buffers[4];

        Buffers[0].pvBuffer = this->Buffer.Buffer;
        Buffers[0].cbBuffer = this->Buffer.Size;
        Buffers[0].BufferType = SECBUFFER_DATA;

        Buffers[1].BufferType = SECBUFFER_EMPTY;
        Buffers[2].BufferType = SECBUFFER_EMPTY;
        Buffers[3].BufferType = SECBUFFER_EMPTY;

    SecBufferDesc BuffersDesc;

        BuffersDesc.cBuffers = 4;
        BuffersDesc.pBuffers = Buffers;
        BuffersDesc.ulVersion = SECBUFFER_VERSION;

    /* You're not allowed to call DecryptMessage with the same context from multiple threads
       at the same time, but since only one thread can process data from a client at a time
       and each context is only for one client, this isn't a problem */

    Status = DecryptMessage(&Context, &BuffersDesc, 0, 0);

    if(Status == SEC_E_INCOMPLETE_MESSAGE)
        return;
    
    this->Buffer.Size = 0;

    if(Status == _HRESULT_TYPEDEF_ (0x00090317L)) /* SEC_I_CONTENT_EXPIRED */
    {
        Client.Public.Disconnect();
        return;
    }
 
    if(Status == SEC_I_RENEGOTIATE)
    {
        /* TODO "The DecryptMessage (Schannel) function returns SEC_I_RENEGOTIATE when the message sender wants
        to renegotiate the connection (security context). An application handles a requested renegotiation
        by calling AcceptSecurityContext (Schannel)  (server side) or InitializeSecurityContext (Schannel)
        (client side) and passing in empty input buffers. After this initial call returns a value, proceed as
        though your application were creating a new connection. For more information, see Creating an Schannel
        Security Context" - http://msdn.microsoft.com/en-us/library/aa374781%28v=VS.85%29.aspx */
    }

    if(FAILED(Status))
    {
        /* Error decrypting the message */

        Lacewing::Error Error;
        Error.Add(Status);
        DebugOut("Error decrypting the message: %s", Error.ToString ());

        Client.Public.Disconnect();
        return;
    }

    SecBuffer * DataBuffer = 0;

    for(int i = 0; i < 4; ++ i)
    {
        SecBuffer &Current = Buffers[i];

        if(Current.BufferType == SECBUFFER_DATA)
        {
            DataBuffer = &Current;
            break;
        }
    }

    if(!DataBuffer || !DataBuffer->cbBuffer)
        return;

    Size = DataBuffer->cbBuffer;
    Buffer = (char *) DataBuffer->pvBuffer;

    for(int i = 0; i < 4; ++i)
    {
        SecBuffer &Current = Buffers[i];

        if(Current.BufferType == SECBUFFER_EXTRA && Current.cbBuffer > 0)
        {
            Extra = (char *) Current.pvBuffer;
            ExtraSize = Current.cbBuffer;

            break;
        }
    }
}

void SecureClientInternal::ProcessHandshakeData(char * Buffer, unsigned int &Size)
{
    if(Status != SEC_I_CONTINUE_NEEDED && Status != SEC_E_INCOMPLETE_MESSAGE
                    && Status != SEC_I_INCOMPLETE_CREDENTIALS)
        return;

    this->Buffer.Add(Buffer, Size);

    SecBuffer InBuffers[2];

        InBuffers[0].BufferType = SECBUFFER_TOKEN;
        InBuffers[0].pvBuffer = this->Buffer.Buffer;
        InBuffers[0].cbBuffer = this->Buffer.Size;

        InBuffers[1].BufferType = SECBUFFER_EMPTY;
        InBuffers[1].pvBuffer = 0;
        InBuffers[1].cbBuffer = 0;

    SecBuffer OutBuffers[2];

        OutBuffers[0].BufferType = SECBUFFER_TOKEN;
        OutBuffers[0].pvBuffer = 0;
        OutBuffers[0].cbBuffer = 0;

        OutBuffers[1].BufferType = SECBUFFER_EMPTY;
        OutBuffers[1].pvBuffer = 0;
        OutBuffers[1].cbBuffer = 0;

    SecBufferDesc InBufferDesc, OutBufferDesc;

        InBufferDesc.ulVersion = SECBUFFER_VERSION;
        InBufferDesc.pBuffers = InBuffers;
        InBufferDesc.cBuffers = 2;

        OutBufferDesc.ulVersion = SECBUFFER_VERSION;
        OutBufferDesc.pBuffers = OutBuffers;
        OutBufferDesc.cBuffers = 2;

    int Flags = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT | ASC_REQ_CONFIDENTIALITY |
                    ASC_REQ_EXTENDED_ERROR | ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM;

    unsigned long SSPIOutFlags;
    TimeStamp ExpiryTime;

    Status = AcceptSecurityContext(&Client.Server.ServerCreds, GotContext ? &Context : 0,
        &InBufferDesc, Flags, SECURITY_NATIVE_DREP, GotContext ? 0 : &Context,
        &OutBufferDesc, &SSPIOutFlags, &ExpiryTime);

    GotContext = true;
    
    if(FAILED (Status))
    {
        /* AcceptSecurityContext failed */

        if(Status != SEC_E_INCOMPLETE_MESSAGE)
        {
            Lacewing::Error Error;
            
            Error.Add(WSAGetLastError ());
            Error.Add("Secure handshake failure");
            
            if (Client.Server.Handlers.Error)
                Client.Server.Handlers.Error(Client.Server.Public, Error);

            Client.Public.Disconnect();

            return;
        }
    }

    if (Status == SEC_E_OK || Status == SEC_I_CONTINUE_NEEDED)
    {
        /* Did AcceptSecurityContext give us back a response to send? */

        if(OutBuffers[0].cbBuffer && OutBuffers[0].pvBuffer)
        {
            /* Send the response to the client and free it */
            
            Client.Public.Send ((char *) OutBuffers [0].pvBuffer, OutBuffers [0].cbBuffer);
            FreeContextBuffer (OutBuffers [0].pvBuffer);
        }

        if(InBuffers[1].BufferType == SECBUFFER_EXTRA)
        {
            Size = InBuffers [1].cbBuffer;
            memmove (Buffer, this->Buffer.Buffer + (this->Buffer.Size - Size), Size);
        }
        else
            Size = 0;

        this->Buffer.Reset();

        if(Status == SEC_E_OK)
        {
            /* Handshake complete! */

            /* Find out the max message size and how big the header/trailer will be */
            
            if((Status = QueryContextAttributes(&Context, SECPKG_ATTR_STREAM_SIZES, &Sizes)) != SEC_E_OK)
            {
                Lacewing::Error Error;
                
                Error.Add(WSAGetLastError ());
                Error.Add("Secure handshake failure");

                if (Client.Server.Handlers.Error)
                    Client.Server.Handlers.Error (Client.Server.Public, Error);

                Client.Public.Disconnect();
            }

            HandshakeComplete = true;
        }
    }

}

int Server::ClientCount ()
{
    return internal->Clients.Size;
}

lw_i64 Server::BytesSent ()
{
    return internal->BytesSent;
}

lw_i64 Server::BytesReceived ()
{
    return internal->BytesReceived;
}

void Server::DisableNagling ()
{
    if (internal->Socket != -1)
    {
        Lacewing::Error Error;
        Error.Add ("DisableNagling() can only be called when the server is not hosting");

        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    internal->Nagle = false;
}

Server::Client * Server::Client::Next ()
{
    return internal->Element->Next ? &(** internal->Element->Next)->Public : 0;
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

