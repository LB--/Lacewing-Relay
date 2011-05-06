
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

#ifndef LacewingFrameBuilder
#define LacewingFrameBuilder

class FrameBuilder : public MessageBuilder
{
protected:

    void PrepareForTransmission()
    {
        if(ToSend)
            return;

        int Type = *(unsigned int *) Buffer;
        int MessageSize = Size - 8;

        int HeaderSize;

        if(MessageSize < 254)
        {
            Buffer[6] = Type;
            Buffer[7] = MessageSize;

            HeaderSize = 2;
        }
        else if(MessageSize < 0xFFFF)
        {
            Buffer[4] = Type;

            (*(unsigned char  *) (Buffer + 5)) = 254;
            (*(unsigned short *) (Buffer + 6)) = MessageSize;

            HeaderSize = 4;
        }
        else if(MessageSize < 0xFFFFFFFF)
        {
            Buffer[2] = Type;

            (*(unsigned char  *) (Buffer + 3)) = 255;
            (*(unsigned int   *) (Buffer + 4)) = MessageSize;

            HeaderSize = 6;
        }

        ToSend     = (Buffer + 8) - HeaderSize;
        ToSendSize =  MessageSize + HeaderSize;
    }

    bool IsUDPClient;

    char * ToSend;
    int ToSendSize;

public:

    FrameBuilder(bool IsUDPClient)
    {
        this->IsUDPClient = IsUDPClient;
        ToSend = 0;
    }

    inline void AddHeader(unsigned char Type, unsigned char Variant, bool ForUDP = false, int UDPClientID = -1)
    {
        LacewingAssert(Size == 0);

        if(!ForUDP)
        {
            Add <unsigned int> ((Type << 4) | Variant);
            Add <unsigned int> (0);

            return;
        }

        Add <unsigned char>  ((Type << 4) | Variant);

        if(IsUDPClient)
            Add <unsigned short> (UDPClientID);
    }

    inline void Send(Lacewing::Server::Client &Client, bool Clear = true)
    {
        PrepareForTransmission ();
        Client.Send            (ToSend, ToSendSize);

        if(Clear)
            FrameReset();
    }

    inline void Send(Lacewing::Client &Client, bool Clear = true)
    {
        PrepareForTransmission ();
        Client.Send            (ToSend, ToSendSize);

        if(Clear)
            FrameReset();
    }

    inline void Send(Lacewing::UDP &UDP, Lacewing::Address &Address, bool Clear = true)
    {
        UDP.Send (Address, Buffer, Size);
 
        if(Clear)
            FrameReset();
    }

    inline void FrameReset()
    {
        Reset();
        ToSend = 0;
    }

};

#endif

