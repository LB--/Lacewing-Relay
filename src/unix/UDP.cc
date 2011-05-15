
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

struct UDPInternal
{
    EventPumpInternal &EventPump;

    Lacewing::UDP &Public;
    
    UDPInternal(Lacewing::UDP &_Public, EventPumpInternal &_EventPump)
            : Public(_Public), EventPump(_EventPump)
    {
        RemoteIP       = 0;

        HandlerReceive = 0;
        HandlerError   = 0;

        Socket = -1;
    }

    Lacewing::UDP::HandlerReceive  HandlerReceive;
    Lacewing::UDP::HandlerError    HandlerError;

    int RemoteIP;
    int Port;

    int Socket;

    lw_i64 BytesSent;
    lw_i64 BytesReceived;
};

void UDPSocketCompletion(UDPInternal &Internal, bool)
{
    sockaddr_in From;
    socklen_t FromSize = sizeof(From);

    char Buffer[256 * 1024];
    
    for(;;)
    {
        int Bytes = recvfrom(Internal.Socket, Buffer, sizeof(Buffer), 0, (sockaddr *) &From, &FromSize);

        if(Bytes == -1)
            break;

        if(Internal.RemoteIP && From.sin_addr.s_addr != Internal.RemoteIP)
            break;

        Lacewing::Address Address(From.sin_addr.s_addr, ntohs(From.sin_port));
        Buffer[Bytes] = 0;

        if(Internal.HandlerReceive)
            Internal.HandlerReceive(Internal.Public, Address, Buffer, Bytes);
    }
}

void Lacewing::UDP::Host(int Port)
{
    Lacewing::Filter Filter;
    Filter.LocalPort(Port);

    Host(Filter);
}

void Lacewing::UDP::Host(Lacewing::Address &Address)
{
    Lacewing::Filter Filter;
    Filter.Remote(Address);

    Host(Filter);
}

void Lacewing::UDP::Host(Lacewing::Filter &Filter)
{
    Unhost();

    UDPInternal &Internal = *((UDPInternal *) InternalTag);

    if(Internal.Socket != -1)
    {
        Lacewing::Error Error;
        Error.Add("Already hosting");
        
        if(Internal.HandlerError)
            Internal.HandlerError(*this, Error);

        return;    
    }

    Internal.Socket   = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    Internal.RemoteIP = Filter.Remote().IP();

    fcntl(Internal.Socket, F_SETFL, fcntl(Internal.Socket, F_GETFL, 0) | O_NONBLOCK);

    Internal.EventPump.AddRead(Internal.Socket, &Internal, (void *) UDPSocketCompletion);

    sockaddr_in SocketAddress;

    memset(&SocketAddress, 0, sizeof(Address));

    SocketAddress.sin_family = AF_INET;
    SocketAddress.sin_port = htons(Filter.LocalPort() ? Filter.LocalPort() : 0);
    SocketAddress.sin_addr.s_addr = Filter.LocalIP() ? Filter.LocalIP() : INADDR_ANY;

    if(bind(Internal.Socket, (sockaddr *) &SocketAddress, sizeof(sockaddr_in)) == -1)
    {
        Lacewing::Error Error;
        
        Error.Add(errno);
        Error.Add("Error binding port");

        if(Internal.HandlerError)
            Internal.HandlerError(*this, Error);

        return;
    }

    socklen_t AddressLength = sizeof(sockaddr_in);
    getsockname(Internal.Socket, (sockaddr *) &SocketAddress, &AddressLength);

    Internal.Port = ntohs(SocketAddress.sin_port);
}

int Lacewing::UDP::Port()
{
    return ((UDPInternal *) InternalTag)->Port;
}

void Lacewing::UDP::Unhost()
{
    UDPInternal &Internal = *((UDPInternal *) InternalTag);

    LacewingCloseSocket(Internal.Socket);
    Internal.Socket = -1;
}

Lacewing::UDP::UDP(Lacewing::EventPump &EventPump)
{
    LacewingInitialise();  
    InternalTag = new UDPInternal(*this, *(EventPumpInternal *) EventPump.InternalTag);
}

Lacewing::UDP::~UDP()
{
    delete ((UDPInternal *) InternalTag);
}

void Lacewing::UDP::Send(Lacewing::Address &Address, const char * Data, int Size)
{
    UDPInternal &Internal = *(UDPInternal *) InternalTag;

    if(!Address.Ready())
    {
        Lacewing::Error Error;

        Error.Add("The address object passed to Send() wasn't ready");
        Error.Add("Error sending");

        if(Internal.HandlerError)
            Internal.HandlerError(Internal.Public, Error);

        return;
    }

    if(Size == -1)
        Size = strlen(Data);

    sockaddr_in To;
    GetSockaddr(Address, To);

    if(sendto(Internal.Socket, Data, Size, 0, (sockaddr *) &To, sizeof(To)) == -1)
    {
        Lacewing::Error Error;

        Error.Add(errno);            
        Error.Add("Error sending");

        if(Internal.HandlerError)
            Internal.HandlerError(*this, Error);

        return;
    }
}

lw_i64 Lacewing::UDP::BytesReceived()
{
    return ((UDPInternal *) InternalTag)->BytesReceived;
}

lw_i64 Lacewing::UDP::BytesSent()
{
    return ((UDPInternal *) InternalTag)->BytesSent;
}

AutoHandlerFunctions(Lacewing::UDP, UDPInternal, Error)
AutoHandlerFunctions(Lacewing::UDP, UDPInternal, Receive)

