
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

struct UDP::Internal
{
    Lacewing::Pump &Pump;

    UDP &Public;
    
    Internal (UDP &_Public, Lacewing::Pump &_Pump)
              : Public (_Public), Pump (_Pump)
    {
        Socket = -1;

        memset (&Handlers, 0, sizeof (Handlers));
    }

    struct
    {
        HandlerReceive Receive;
        HandlerError Error;

    } Handlers;

    Lacewing::Filter Filter;

    int Socket;

    lw_i64 BytesSent, BytesReceived;
};

static void ReadReady (UDP::Internal * internal, bool)
{
    sockaddr_storage From;
    socklen_t FromSize = sizeof (From);

    char Buffer[256 * 1024];
    
    for(;;)
    {
        int Bytes = recvfrom (internal->Socket, Buffer, sizeof (Buffer), 0, (sockaddr *) &From, &FromSize);

        if(Bytes == -1)
            break;

        AddressWrapper Address;                
        Address.Set (&From);

        Lacewing::Address * FilterAddress = internal->Filter.Remote ();

        if (FilterAddress && ((Lacewing::Address) Address) != *FilterAddress)
            break;

        Buffer[Bytes] = 0;

        if (internal->Handlers.Receive)
            internal->Handlers.Receive (internal->Public, Address, Buffer, Bytes);
    }
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
    Filter.Remote(&Address);

    Host(Filter);
}

void UDP::Host (Filter &Filter)
{
    Unhost ();

    {   Lacewing::Error Error;

        if ((internal->Socket = CreateServerSocket (Filter, SOCK_DGRAM, IPPROTO_UDP, Error)) == -1)
        {
            if (internal->Handlers.Error)
                internal->Handlers.Error (*this, Error);

            return;    
        }
    }

    internal->Pump.Add (internal->Socket, internal, (Pump::Callback) ReadReady);
}

bool UDP::Hosting ()
{
    return internal->Socket != -1;
}

int UDP::Port ()
{
    return GetSocketPort (internal->Socket);
}

void UDP::Unhost ()
{
    LacewingCloseSocket (internal->Socket);
    internal->Socket = -1;
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
    if(!Address.Ready())
    {
        Lacewing::Error Error;

        Error.Add("The address object passed to Send() wasn't ready");
        Error.Add("Error sending");

        if (internal->Handlers.Error)
            internal->Handlers.Error (internal->Public, Error);

        return;
    }

    if(Size == -1)
        Size = strlen(Data);

    addrinfo * Info = Address.internal->Info;

    if (!Info)
        return;

    if (sendto (internal->Socket, Data, Size, 0,
                    (sockaddr *) Info->ai_addr, Info->ai_addrlen) == -1)
    {
        Lacewing::Error Error;

        Error.Add (errno);            
        Error.Add ("Error sending");

        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
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

