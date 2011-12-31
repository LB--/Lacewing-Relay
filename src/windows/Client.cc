
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

struct ClientOverlapped
{
    OVERLAPPED Overlapped;
    bool IsSend;
};

struct Client::Internal
{
    Lacewing::Pump &Pump;

    ClientOverlapped Overlapped;

    struct
    {
        HandlerConnect     Connect;
        HandlerDisconnect  Disconnect;
        HandlerReceive     Receive;
        HandlerError       Error;

    } Handlers;

    Client &Public;

    Internal (Client &_Public, Lacewing::Pump &_Pump)
                : Public (_Public), Pump (_Pump)
    {
        Connected = Connecting = false;

        Socket = -1;

        memset (&Handlers, 0, sizeof (Handlers));

        Nagle = true;
        BufferingOutput = false;

        Address = 0;
    }

    ~ Internal ()
    {
        delete Address;
    }

    Lacewing::Address * Address;

    SOCKET Socket;
    bool Connected, Connecting;

    ReceiveBuffer Buffer;
    WSABUF WinsockBuffer;

    void Completion  (ClientOverlapped &Overlapped, unsigned int BytesTransferred, int Error);
    bool PostReceive ();

    bool Nagle;
    
    MessageBuilder OutputBuffer;
    bool BufferingOutput;

    Backlog <ClientOverlapped> OverlappedBacklog;
};

Client::Client (Lacewing::Pump &Pump)
{
    LacewingInitialise ();
    internal = new Client::Internal (*this, Pump);
}

Client::~Client ()
{
    Disconnect ();

    delete internal;
}

void Client::Connect (const char * Host, int Port)
{
    Address Address (Host, Port, true);
    Connect (Address);
}

void Completion (Client::Internal * internal, ClientOverlapped &Overlapped, unsigned int BytesTransferred, int Error)
{
    internal->Completion (Overlapped, BytesTransferred, Error);
}

void Client::Internal::Completion (ClientOverlapped &Overlapped, unsigned int BytesTransferred, int Error)
{
    if(Overlapped.IsSend)
    {
        OverlappedBacklog.Return(Overlapped);
        return;
    }

    if (Connecting)
    {
        if(!PostReceive ())
        {
            /* Failed to connect */

            Connecting = false;

            Lacewing::Error Error;
            Error.Add("Error connecting");

            if (Handlers.Error)
                Handlers.Error (Public, Error);

            return;
        }

        Connected  = true;
        Connecting = false;

        if (Handlers.Connect)
            Handlers.Connect (Public);

        return;
    }

    if (!BytesTransferred)
    {
        Socket = -1;
        Connected = false;

        if (Handlers.Disconnect)
            Handlers.Disconnect (Public);

        return;
    }

    Buffer.Received(BytesTransferred);

    if (Handlers.Receive)
        Handlers.Receive (Public, Buffer.Buffer, BytesTransferred);
    
    if (Socket == -1)
    {
        /* Disconnect called from within the receive handler */

        Connected = false;
        
        if (Handlers.Disconnect)
            Handlers.Disconnect (Public);

        return;
    }

    PostReceive();
}

bool Client::Internal::PostReceive ()
{
    memset(&Overlapped, 0, sizeof(ClientOverlapped));
    Overlapped.IsSend = false;

    Buffer.Prepare();

    WinsockBuffer.len = Buffer.Size;
    WinsockBuffer.buf = Buffer.Buffer;

    DWORD Flags = 0;

    if (WSARecv (Socket, &WinsockBuffer, 1, 0, &Flags, (OVERLAPPED *) &Overlapped, 0) == -1)
    {
        int Code = LacewingGetSocketError ();

        return (Code == WSA_IO_PENDING);
    }

    return true;
}

void Client::Connect (Address &Address)
{
    if (Connected () || Connecting ())
    {
        Lacewing::Error Error;
        Error.Add("Already connected to a server");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    internal->Connecting = true;

    /* TODO : Resolve asynchronously? */

    {   Error * Error = Address.Resolve ();

        if (Error)
        {
            if (internal->Handlers.Error)
                internal->Handlers.Error (*this, *Error);
                
            return;
        }
    }

    if (!Address.internal->Info)
    {
        Lacewing::Error Error;
        Error.Add ("Invalid address");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    if ((internal->Socket = WSASocket
        (AF_INET6, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED)) == -1)
    {
        Lacewing::Error Error;
       
        Error.Add(LacewingGetSocketError ());        
        Error.Add("Error creating socket");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    DisableIPV6Only (internal->Socket);

    if(!internal->Nagle)
        ::DisableNagling(internal->Socket);

    internal->Pump.Add ((HANDLE) internal->Socket, internal, (Pump::Callback) Completion);

    memset (&internal->Overlapped, 0, sizeof (OVERLAPPED));
    internal->Overlapped.IsSend = false;

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

    ) = 0;

    if (!lw_ConnectEx)
    {   
        GUID  ID = {0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}};
        DWORD Bytes = 0;

        WSAIoctl (internal->Socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &ID,
                        sizeof (ID), &lw_ConnectEx, sizeof (lw_ConnectEx), &Bytes, 0, 0);
    }

    {   sockaddr_storage LocalAddress;
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

        if (bind (internal->Socket, (sockaddr *) &LocalAddress, sizeof (LocalAddress)) == -1)
        {
            Lacewing::Error Error;
           
            Error.Add (LacewingGetSocketError ());        
            Error.Add ("Error binding socket");
            
            if (internal->Handlers.Error)
                internal->Handlers.Error (*this, Error);

            return;
        }
    }

    delete internal->Address;
    internal->Address = new Lacewing::Address (Address);

    if (!lw_ConnectEx (internal->Socket, Address.internal->Info->ai_addr,
            Address.internal->Info->ai_addrlen, 0, 0, 0, (OVERLAPPED *) &internal->Overlapped))
    {
        int Code = LacewingGetSocketError();

        LacewingAssert(Code == WSA_IO_PENDING);
    }
}

bool Client::Connected ()
{
    return internal->Connected;
}

bool Client::Connecting ()
{
    return internal->Connecting;
}

void Client::Disconnect ()
{
    LacewingCloseSocket (internal->Socket);
    internal->Socket = -1;
}

void Client::Send (const char * Data, int Size)
{
    if (!internal->Connected)
        return;

    if(Size == -1)
        Size = strlen(Data);

    WSABUF SendBuffer = { Size, (CHAR *) Data };

    ClientOverlapped &Overlapped = internal->OverlappedBacklog.Borrow ();    

    memset (&Overlapped, 0, sizeof (OVERLAPPED));
    Overlapped.IsSend = true;

    if (WSASend (internal->Socket, &SendBuffer, 1, 0, 0, (OVERLAPPED *) &Overlapped, 0) != 0)
    {
        int Error = WSAGetLastError();

        if(Error != WSA_IO_PENDING)
        {
            internal->OverlappedBacklog.Return (Overlapped);
        }
    }
}

Address &Client::ServerAddress ()
{
    return *internal->Address;
}

void Client::DisableNagling ()
{
    internal->Nagle = false;

    if (internal->Socket != -1)
        ::DisableNagling (internal->Socket);
}

bool Client::CheapBuffering ()
{
    return false;
}

void Client::StartBuffering ()
{
    if (internal->BufferingOutput)
        return;

    internal->BufferingOutput = true;
}

void Client::Flush ()
{
    if (!internal->BufferingOutput)
        return;

    internal->BufferingOutput = false;
    Send (internal->OutputBuffer.Buffer, internal->OutputBuffer.Size);

    internal->OutputBuffer.Reset ();
}

AutoHandlerFunctions (Client, Connect)
AutoHandlerFunctions (Client, Disconnect)
AutoHandlerFunctions (Client, Receive)
AutoHandlerFunctions (Client, Error)

