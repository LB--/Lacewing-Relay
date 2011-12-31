
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

const int IdealPendingReceiveCount = 16;

namespace OverlappedType
{
    enum Type
    {
        Send,
        Receive,
    };
}

struct UDPOverlapped
{
    OVERLAPPED Overlapped;

    OverlappedType::Type Type;
    void * Tag;

    UDPOverlapped ()
    {
        memset (&Overlapped, 0, sizeof (OVERLAPPED));
    }
};

struct UDPReceiveInformation
{
    /* TODO : Replace with ReceiveBuffer? */

    char Buffer [1024 * 32];
    WSABUF WinsockBuffer;

    UDPReceiveInformation ()
    {
        WinsockBuffer.buf = Buffer;
        WinsockBuffer.len = sizeof(Buffer);

        FromLength = sizeof (sockaddr_storage);
    }

    sockaddr_storage From;
    int FromLength;
};

struct UDP::Internal
{
    Lacewing::Pump &Pump;

    UDP &Public;
    
    Internal (UDP &_Public, Lacewing::Pump &_Pump)
            : Public (_Public), Pump (_Pump)
    {
        ReceivesPosted = 0;

        memset (&Handlers, 0, sizeof (Handlers));

        Socket = -1;
    }

    struct
    {
        HandlerReceive Receive;
        HandlerError Error;

    } Handlers;

    Lacewing::Filter Filter;

    int Port;

    SOCKET Socket;

    lw_i64 BytesSent;
    lw_i64 BytesReceived;

    Backlog <UDPOverlapped> OverlappedBacklog;
    Backlog <UDPReceiveInformation> ReceiveInformationBacklog;

    volatile long ReceivesPosted;

    void PostReceives()
    {
        while(ReceivesPosted < IdealPendingReceiveCount)
        {
            UDPReceiveInformation &ReceiveInformation = ReceiveInformationBacklog.Borrow ();
            UDPOverlapped &Overlapped = OverlappedBacklog.Borrow ();

            Overlapped.Type = OverlappedType::Receive;
            Overlapped.Tag = &ReceiveInformation;

            DWORD Flags = 0;

            if(WSARecvFrom (Socket, &ReceiveInformation.WinsockBuffer,
                            1, 0, &Flags, (sockaddr *) &ReceiveInformation.From, &ReceiveInformation.FromLength,
                            (OVERLAPPED *) &Overlapped, 0) == -1)
            {
                int Error = WSAGetLastError();

                if(Error != WSA_IO_PENDING)
                    break;
            }

            InterlockedIncrement(&ReceivesPosted);
        }
    }
};

static void UDPSocketCompletion (UDP::Internal * internal, UDPOverlapped &Overlapped, unsigned int BytesTransferred, int Error)
{
    switch(Overlapped.Type)
    {
        case OverlappedType::Send:
        {
            internal->BytesSent += BytesTransferred;
            break;
        }

        case OverlappedType::Receive:
        {
            internal->BytesReceived += BytesTransferred;

            UDPReceiveInformation &ReceiveInformation = *(UDPReceiveInformation *) Overlapped.Tag;

            ReceiveInformation.Buffer [BytesTransferred] = 0;

            {   AddressWrapper Address;                
                Address.Set (&ReceiveInformation.From);

                Lacewing::Address * FilterAddress = internal->Filter.Remote ();

                if (FilterAddress && ((Lacewing::Address) Address) != *FilterAddress)
                    break;

                if (internal->Handlers.Receive)
                    internal->Handlers.Receive (internal->Public, (Lacewing::Address &) Address,
                                                        ReceiveInformation.Buffer, BytesTransferred);
            }

            internal->ReceiveInformationBacklog.Return (ReceiveInformation);

            InterlockedDecrement(&internal->ReceivesPosted);
            internal->PostReceives();

            break;
        }
    };

    internal->OverlappedBacklog.Return (Overlapped);
}

void UDP::Host (int Port)
{
    Filter Filter;
    Filter.LocalPort(Port);

    Host(Filter);
}

void UDP::Host (Address &Address)
{
    Filter Filter;
    Filter.Remote (&Address);

    Host(Filter);
}

void UDP::Host (Filter &Filter)
{
    Unhost();

    if (Hosting())
    {
        Lacewing::Error Error;
        Error.Add("Already hosting");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;    
    }

    {   Lacewing::Error Error;

        if ((internal->Socket = CreateServerSocket
                (Filter, SOCK_DGRAM, IPPROTO_UDP, Error)) == -1)
        {
            if (internal->Handlers.Error)
                internal->Handlers.Error (*this, Error);

            return;
        }
    }

    internal->Filter.~Filter ();
    new (&internal->Filter) Lacewing::Filter (Filter);

    internal->Pump.Add ((HANDLE) internal->Socket,
            internal, (Lacewing::Pump::Callback) UDPSocketCompletion);

    internal->Port = GetSocketPort (internal->Socket);

    internal->PostReceives ();
}

bool UDP::Hosting ()
{
    return internal->Socket != -1;
}

void UDP::Unhost ()
{
    LacewingCloseSocket (internal->Socket);
    internal->Socket = -1;
}

int UDP::Port ()
{
    return internal->Port;
}

UDP::UDP (Lacewing::Pump &Pump)
{
    LacewingInitialise ();  
    internal = new UDP::Internal (*this, Pump);
}

UDP::~UDP ()
{
    delete internal;
}

void UDP::Send (Address &Address, const char * Data, int Size)
{
    if ((!Address.Ready ()) || !Address.internal->Info)
    {
        Lacewing::Error Error;

        Error.Add("The address object passed to Send () wasn't ready");        
        Error.Add("Error sending");

        if (internal->Handlers.Error)
            internal->Handlers.Error (internal->Public, Error);

        return;
    }

    if (Size == -1)
        Size = strlen (Data);

    WSABUF Buffer = { Size, (CHAR *) Data };

    UDPOverlapped &Overlapped = internal->OverlappedBacklog.Borrow ();

    Overlapped.Type = OverlappedType::Send;
    Overlapped.Tag  = 0;

    addrinfo * Info = Address.internal->Info;

    if (!Info)
        return;

    if (WSASendTo (internal->Socket, &Buffer, 1, 0, 0, Info->ai_addr,
                        Info->ai_addrlen, (OVERLAPPED *) &Overlapped, 0) == -1)
    {
        int Code = WSAGetLastError();

        if(Code != WSA_IO_PENDING)
        {
            Lacewing::Error Error;

            Error.Add(Code);            
            Error.Add("Error sending");

            if (internal->Handlers.Error)
                internal->Handlers.Error (*this, Error);

            return;
        }
    }
}

lw_i64 UDP::BytesReceived ()
{
    return internal->BytesReceived;
}

lw_i64 UDP::BytesSent ()
{
    return internal->BytesSent;
}

AutoHandlerFunctions (UDP, Error)
AutoHandlerFunctions (UDP, Receive)

