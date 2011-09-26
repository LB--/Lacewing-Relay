
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

#include "Common.h"

struct FlashPlayerPolicyInternal
{
    char * Buffer;
    size_t Size;

    Lacewing::FlashPlayerPolicy &Public;
    Lacewing::Server &Socket;

    Lacewing::FlashPlayerPolicy::HandlerError HandlerError;

    FlashPlayerPolicyInternal(Lacewing::FlashPlayerPolicy &_Public, Lacewing::Server &_Socket)
            : Public(_Public), Socket(_Socket)
    {
        Buffer        = 0;
        HandlerError  = 0;
    }
};

void SocketReceive(Lacewing::Server &Socket, Lacewing::Server::Client &Client, char * Buffer, int Size)
{
    FlashPlayerPolicyInternal &Internal = *(FlashPlayerPolicyInternal *) Socket.Tag;

    for(int i = 0; i < Size; ++i)
    {
        if(!Buffer[i])
        {
            Client.Send(Internal.Buffer, Internal.Size);
            Client.Send("\0", 1);

            return;
        }
    }
}

void SocketError(Lacewing::Server &Socket, Lacewing::Error &Error)
{
    FlashPlayerPolicyInternal &Internal = *(FlashPlayerPolicyInternal *) Socket.Tag;

    Error.Add("Socket error");
    
    if(Internal.HandlerError)
        Internal.HandlerError(Internal.Public, Error);
}

Lacewing::FlashPlayerPolicy::FlashPlayerPolicy(EventPump &EventPump) : Socket(EventPump)
{
    Socket.Tag = InternalTag = new FlashPlayerPolicyInternal(*this, Socket);

    Socket.onError   (SocketError);
    Socket.onReceive (SocketReceive);
}

Lacewing::FlashPlayerPolicy::~FlashPlayerPolicy()
{
    Unhost();
    
    delete ((FlashPlayerPolicyInternal *) InternalTag);
}

void Lacewing::FlashPlayerPolicy::Host(const char * Filename, int Port)
{
    Lacewing::Filter Filter;
    Filter.LocalPort(Port);

    Host(Filename, Filter);
}

void Lacewing::FlashPlayerPolicy::Host(const char * Filename, Lacewing::Filter &Filter)
{
    Unhost();

    if(!Filter.LocalPort())
        Filter.LocalPort(843);
    
    FlashPlayerPolicyInternal &Internal = *(FlashPlayerPolicyInternal *) InternalTag;
    
    {   FILE * File = fopen(Filename, "r");

        if(!File)
        {
            Lacewing::Error Error;

            Error.Add (LacewingGetLastError());
            Error.Add ("Error opening file: %s", Filename);
                
            if(Internal.HandlerError)
                Internal.HandlerError(*this, Error);

            return;
        }

        fseek(File, 0, SEEK_END);

        Internal.Size = ftell(File);
        Internal.Buffer = (char *) malloc(Internal.Size);
        
        fseek(File, 0, SEEK_SET);

        int bytes = fread (Internal.Buffer, 1, Internal.Size, File);
        
        if (bytes != Internal.Size)
        {
            Internal.Size = bytes;

            if (ferror (File))
            {
                Lacewing::Error Error;
                
                Error.Add (LacewingGetLastError());
                Error.Add ("Error reading file: %s", Filename);

                if(Internal.HandlerError)
                    Internal.HandlerError(*this, Error);

                free (Internal.Buffer);
                Internal.Buffer = 0;
        
                fclose (File);
                
                return;
            }
        }

        fclose(File);
    }

    Socket.Host(Filter);
}

void Lacewing::FlashPlayerPolicy::Unhost()
{
    Socket.Unhost();
    
    FlashPlayerPolicyInternal &Internal = *(FlashPlayerPolicyInternal *) InternalTag;
    free(Internal.Buffer);
}

bool Lacewing::FlashPlayerPolicy::Hosting()
{
    return Socket.Hosting();
}

AutoHandlerFunctions(Lacewing::FlashPlayerPolicy, FlashPlayerPolicyInternal, Error);

