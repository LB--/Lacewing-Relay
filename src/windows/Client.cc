
/*
    Copyright (C) 2011 James McLaughlin

    This file is part of Lacewing.

    Lacewing is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lacewing is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Lacewing.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "../Common.h"

struct ClientInternal;

struct ClientInternal
{
    EventPumpInternal  &EventPump;

    OVERLAPPED Overlapped;

    Lacewing::Client::HandlerConnect     HandlerConnect;
    Lacewing::Client::HandlerDisconnect  HandlerDisconnect;
    Lacewing::Client::HandlerReceive     HandlerReceive;
    Lacewing::Client::HandlerError       HandlerError;

    Lacewing::Client &Public;

    ClientInternal(Lacewing::Client &_Public, EventPumpInternal &_EventPump)
            : Public(_Public), EventPump(_EventPump)
    {
        Connected = Connecting = false;

        Address = 0;
        Socket  = SOCKET_ERROR;

        HandlerConnect     = 0;
        HandlerDisconnect  = 0;
        HandlerReceive     = 0;
        HandlerError       = 0;

        WinsockBuffer.len = sizeof(Buffer) - 1;
        WinsockBuffer.buf = Buffer;

        Nagle = true;
        BufferingOutput = false;

        Address = new Lacewing::Address();
    }

    Lacewing::Address * Address;

    SOCKET Socket;
    bool Connected, Connecting;

    sockaddr_in HostStructure;

    char   Buffer[1024 * 256];
    WSABUF WinsockBuffer;

    void Completion  (unsigned int BytesTransferred, int Error);
    bool PostReceive ();

    bool Nagle;
    
    MessageBuilder OutputBuffer;
    bool BufferingOutput;
};

Lacewing::Client::Client(Lacewing::EventPump &EventPump)
{
    LacewingInitialise();
    InternalTag = new ClientInternal(*this, *(EventPumpInternal *) EventPump.InternalTag);
}

Lacewing::Client::~Client()
{
    Disconnect();

    delete ((ClientInternal *) InternalTag);
}

void Lacewing::Client::Connect(const char * Host, int Port)
{
    if(!Port)
        Port = 6121;

    Lacewing::Address Address(Host, Port, true);
    Connect(Address);
}

void Completion(ClientInternal &Internal, OVERLAPPED &, unsigned int BytesTransferred, int Error)
{
    Internal.Completion(BytesTransferred, Error);
}

void ClientInternal::Completion(unsigned int BytesTransferred, int Error)
{
    if(Connecting)
    {
        if(!PostReceive())
        {
            /* Failed to connect */

            Connecting = false;

            Lacewing::Error Error;
            Error.Add("Error connecting");

            if(HandlerError)
                HandlerError(Public, Error);

            return;
        }

        Connected  = true;
        Connecting = false;

        if(HandlerConnect)
            HandlerConnect(Public);

        return;
    }

    if(!BytesTransferred)
    {
        Socket = SOCKET_ERROR;
        Connected = false;

        if(HandlerDisconnect)
            HandlerDisconnect(Public);

        return;
    }

    Buffer[BytesTransferred] = 0;

    if(HandlerReceive)
        HandlerReceive(Public, Buffer, BytesTransferred);
    
    if(Socket == SOCKET_ERROR)
    {
        /* Disconnect called from within the receive handler */

        Connected = false;
        
        if(HandlerDisconnect)
            HandlerDisconnect(Public);

        return;
    }

    PostReceive();
}

bool ClientInternal::PostReceive()
{
    memset(&Overlapped, 0, sizeof(OVERLAPPED));

    DWORD Flags = 0;

    if(WSARecv(Socket, &WinsockBuffer, 1, 0, &Flags, &Overlapped, 0) == SOCKET_ERROR)
    {
        int Code = LacewingGetSocketError();

        return (Code == WSA_IO_PENDING);
    }

    return true;
}

void Lacewing::Client::Connect(Lacewing::Address &Address)
{
    ClientInternal &Internal = *((ClientInternal *) InternalTag);
    
    if(Connected() || Connecting())
    {
        Lacewing::Error Error;
        Error.Add("Already connected to a server");
        
        if(Internal.HandlerError)
            Internal.HandlerError(*this, Error);

        return;
    }

    Internal.Connecting = true;

    while(!Address.Ready())
        LacewingYield();

    delete Internal.Address;
    Internal.Address = new Lacewing::Address(Address);

    GetSockaddr(*Internal.Address, Internal.HostStructure);

    if((Internal.Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED)) == SOCKET_ERROR)
    {
        Lacewing::Error Error;
       
        Error.Add(LacewingGetSocketError());        
        Error.Add("Error creating socket");
        
        if(Internal.HandlerError)
            Internal.HandlerError(*this, Error);

        return;
    }

    if(!Internal.Nagle)
        ::DisableNagling(Internal.Socket);

    Internal.EventPump.Add((HANDLE) Internal.Socket, (void *) &Internal, (void *) Completion);

    memset(&Internal.Overlapped, 0, sizeof(OVERLAPPED));

    LPFN_CONNECTEX ConnectEx;
    
    {   GUID  ID    = WSAID_CONNECTEX;
        DWORD Bytes = 0;

        WSAIoctl(Internal.Socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &ID, sizeof(ID), &ConnectEx, sizeof(LPFN_CONNECTEX), &Bytes, 0, 0);
    }

    sockaddr_in LocalAddress;
    memset(&LocalAddress, 0, sizeof(sockaddr_in));

    LocalAddress.sin_family = AF_INET;
    LocalAddress.sin_addr.S_un.S_addr = INADDR_ANY;

    if(bind(Internal.Socket, (sockaddr *) &LocalAddress, sizeof(sockaddr_in)) == SOCKET_ERROR)
    {
        int Code = LacewingGetSocketError();

        LacewingAssert(false);
    }

    if(!ConnectEx(Internal.Socket, (sockaddr *) &Internal.HostStructure, sizeof(sockaddr_in), 0, 0, 0, (OVERLAPPED *) &Internal.Overlapped))
    {
        int Code = LacewingGetSocketError();

        LacewingAssert(Code == WSA_IO_PENDING);
    }
}

bool Lacewing::Client::Connected()
{
    return ((ClientInternal *) InternalTag)->Connected;
}

bool Lacewing::Client::Connecting()
{
    return ((ClientInternal *) InternalTag)->Connecting;
}

void Lacewing::Client::Disconnect()
{
    ClientInternal &Internal = *((ClientInternal *) InternalTag);

    LacewingCloseSocket(Internal.Socket);
    Internal.Socket = SOCKET_ERROR;
}

void Lacewing::Client::Send(const char * Data, int Size)
{
    ClientInternal &Internal = *(ClientInternal *) InternalTag;

    if(!Internal.Connected)
        return;

    if(Size == -1)
        Size = strlen(Data);

    send(Internal.Socket, Data, Size, 0);
}

Lacewing::Address &Lacewing::Client::ServerAddress()
{
    return *((ClientInternal *) InternalTag)->Address;
}

void Lacewing::Client::DisableNagling()
{
    ClientInternal &Internal = *(ClientInternal *) InternalTag;

    Internal.Nagle = false;

    if(Internal.Socket != SOCKET_ERROR)
        ::DisableNagling(Internal.Socket);
}

void Lacewing::Client::StartBuffering()
{
    ClientInternal &Internal = *(ClientInternal *) InternalTag;

    if(Internal.BufferingOutput)
        return;

    Internal.BufferingOutput = true;
}

void Lacewing::Client::Flush()
{
    ClientInternal &Internal = *(ClientInternal *) InternalTag;

    if(!Internal.BufferingOutput)
        return;

    Internal.BufferingOutput = false;
    Send(Internal.OutputBuffer.Buffer, Internal.OutputBuffer.Size);

    Internal.OutputBuffer.Reset();
}

AutoHandlerFunctions(Lacewing::Client, ClientInternal, Connect)
AutoHandlerFunctions(Lacewing::Client, ClientInternal, Disconnect)
AutoHandlerFunctions(Lacewing::Client, ClientInternal, Receive)
AutoHandlerFunctions(Lacewing::Client, ClientInternal, Error)

