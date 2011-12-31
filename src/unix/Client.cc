
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

struct Client::Internal
{
    Lacewing::Pump &Pump;

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

        Address =  0;
        Socket  = -1;

        memset (&Handlers, 0, sizeof (Handlers));

        Nagle = true;
        Address = 0;

        QueuedOffset = 0;
    }

    ~ Internal ()
    {
        delete Address;
    }

    Lacewing::Address * Address;

    int Socket;
    bool Connected, Connecting;

    ReceiveBuffer Buffer;

    void ReadReady ();
    void WriteReady ();

    bool Nagle;

    MessageBuilder Queued;
    int QueuedOffset;
};

Client::Client (Lacewing::Pump &Pump)
{
    LacewingInitialise ();

    internal = new Internal (*this, Pump);
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

void Client::Internal::WriteReady ()
{
    if(Connecting)
    {
        int Error;
        
        {   socklen_t ErrorLength = sizeof (Error);
            getsockopt (Socket, SOL_SOCKET, SO_ERROR, &Error, &ErrorLength);
        }

        if(Error != 0)
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
    }

    if(Queued.Size > 0)
    {
        int Sent = send (Socket, Queued.Buffer + QueuedOffset, Queued.Size - QueuedOffset, LacewingNoSignal);
        
        if(Sent < (Queued.Size - QueuedOffset))
            QueuedOffset += Sent;
        else
        {
            Queued.Reset();
            QueuedOffset = 0;
        }
    }
}

void Client::Internal::ReadReady ()
{
    for(;;)
    {
        Buffer.Prepare ();
        int Bytes = recv (Socket, Buffer.Buffer, Buffer.Size, MSG_DONTWAIT);

        if(Bytes == -1)
        {
            if(errno == EAGAIN)
                break;

            /* Assume some sort of disconnect */

            close(Socket);
            Bytes = 0;
        }

        if(Bytes == 0)
        {
            Socket = -1;
            Connected = false;

            if (Handlers.Disconnect)
                Handlers.Disconnect (Public);

            return;
        }

        Buffer.Received(Bytes);

        if (Handlers.Receive)
            Handlers.Receive (Public, Buffer.Buffer, Bytes);
            
        if(Socket == -1)
        {
            /* Disconnect called from within the receive handler */

            Connected = false;
            
            if (Handlers.Disconnect)
                Handlers.Disconnect (Public);

            return;
        }
    }
}

static void WriteReady (Client::Internal * internal)
{   internal->WriteReady ();
}

static void ReadReady (Client::Internal * internal)
{   internal->ReadReady ();
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

    delete internal->Address;
    internal->Address = new Lacewing::Address (Address);

    if ((internal->Socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        Lacewing::Error Error;
       
        Error.Add(LacewingGetSocketError ());        
        Error.Add("Error creating socket");
        
        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);

        return;
    }

    DisableSigPipe (internal->Socket);

    if (!internal->Nagle)
        ::DisableNagling (internal->Socket);

    fcntl (internal->Socket, F_SETFL, fcntl (internal->Socket, F_GETFL, 0) | O_NONBLOCK);

    internal->Pump.Add (internal->Socket, internal,
                    (Pump::Callback) ReadReady, (Pump::Callback) WriteReady);

    if (connect (internal->Socket, Address.internal->Info->ai_addr,
                        Address.internal->Info->ai_addrlen) == -1)
    {
        if(errno == EINPROGRESS)
            return;

        internal->Connecting = false;

        Lacewing::Error Error;
       
        Error.Add(errno);        
        Error.Add("Error connecting");

        if (internal->Handlers.Error)
            internal->Handlers.Error (*this, Error);
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
    if(!Connected())
        return;

    if (Size == -1)
        Size = strlen (Data);


    if (internal->Queued.Size > 0)
    {
        internal->Queued.Add (Data, Size);
        return;
    }

    int Sent = send (internal->Socket, Data, Size, LacewingNoSignal);

    if(Sent < Size)
        internal->Queued.Add (Data + Sent, Size - Sent);
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
    /* TODO : Userland buffering support for Unix client? */

    return true;
}

void Client::StartBuffering ()
{
    if(!Connected())
        return;

    #ifdef LacewingAllowCork
        int Enabled = 1;
        setsockopt (internal->Socket, IPPROTO_TCP, LacewingCork, &Enabled, sizeof (Enabled));
    #endif
}

void Client::Flush ()
{
    if(!Connected())
        return;

    #ifdef LacewingAllowCork    
        int Enabled = 0;
        setsockopt (internal->Socket, IPPROTO_TCP, LacewingCork, &Enabled, sizeof (Enabled));
    #endif
}

AutoHandlerFunctions (Client, Connect)
AutoHandlerFunctions (Client, Disconnect)
AutoHandlerFunctions (Client, Receive)
AutoHandlerFunctions (Client, Error)

