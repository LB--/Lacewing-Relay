
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

#ifndef LacewingMessageBuilder
#define LacewingMessageBuilder

class MessageBuilder
{
    
protected:

    unsigned int Allocated;

public:
    
    char * Buffer;
    unsigned int Size;

    inline MessageBuilder()
    {
        Size   = Allocated = 0;
        Buffer = 0;
    }

    inline ~MessageBuilder()
    {
        free(Buffer);
    }

    inline void Add(const char * Buffer, int Size)
    {
        if(Size == -1)
            Size = strlen(Buffer);

        if(this->Size + Size > Allocated)
        {
            if(!Allocated)
                Allocated = 1024 * 4;
            else
                Allocated *= 3;

            if(this->Size + Size > Allocated)
                Allocated += Size;

            this->Buffer = (char *) realloc(this->Buffer, Allocated);
        }

        memcpy(this->Buffer + this->Size, Buffer, Size);
        this->Size += Size;
    }

    inline void Add(const string &Text)
    {
        Add(Text.c_str(), Text.length());
    }

    template<class T> inline void Add(T Value)
    {
        Add((const char *) &Value, sizeof(T));
    }

    inline void Reset()
    {
        Size = 0;
    }

    inline void Send(Lacewing::Client &Socket, int Offset = 0)
    {
        Socket.Send(Buffer + Offset, Size - Offset);
    }

    inline void Send(Lacewing::Server::Client &Socket, int Offset = 0)
    {
        Socket.Send(Buffer + Offset, Size - Offset);
    }

    inline void Send(Lacewing::UDP &UDP, Lacewing::Address &Address, int Offset = 0)
    {
        UDP.Send(Address, Buffer + Offset, Size - Offset);
    }

};

#endif

