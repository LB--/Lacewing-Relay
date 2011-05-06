
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

bool SocketConnect(Lacewing::Server &Socket, Lacewing::Server::Client &Client)
{

    return true;
}

void SocketDisconnect(Lacewing::Server &Socket, Lacewing::Server::Client &Client)
{
}

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

void Lacewing::FlashPlayerPolicy::Host(const char * Filename, Lacewing::Filter &_Filter)
{
    Unhost();

    Lacewing::Filter Filter;

    if(!Filter.LocalPort())
        Filter.LocalPort(843);
    
    FlashPlayerPolicyInternal &Internal = *(FlashPlayerPolicyInternal *) InternalTag;
    
    if(!Lacewing::FileExists(Filename))
    {
        Lacewing::Error Error;
        Error.Add("File not found: %s", Filename);
        
        if(Internal.HandlerError)
            Internal.HandlerError(*this, Error);

        return;
    }

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
        Internal.Buffer = (char *) malloc(((FlashPlayerPolicyInternal *)InternalTag)->Size);
        
        fseek(File, 0, SEEK_SET);

        if(fread(Internal.Buffer, 1, Internal.Size, File) != Internal.Size)
        {
            Lacewing::Error Error;
            
            Error.Add (LacewingGetLastError());
            Error.Add ("Error opening file: %s", Filename);

            if(Internal.HandlerError)
                Internal.HandlerError(*this, Error);

            free(Internal.Buffer);
            Internal.Buffer = 0;

            fclose(File);
            
            return;
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

