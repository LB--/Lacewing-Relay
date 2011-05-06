
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

#ifndef LacewingFrameReader
#define LacewingFrameReader

class FrameReader
{
    
protected:

    MessageBuilder Buffer;
        
    int            State;
    int            SizeBytesLeft;
    unsigned int   MessageSize;
    unsigned char  MessageType; 

public:

    void  * Tag;
    void (* MessageHandler) (void * Tag, unsigned char Type, char * Message, int Size);

    FrameReader()
    {
        MessageHandler = 0;
        State          = 0;
    }

    inline void Process(char * Data, unsigned int Size)
    {
        while(State < 3 && Size -- > 0)
        {
            unsigned char Byte = *(Data ++);

            switch(State)
            {
                case 0: /* Haven't yet got type */
                    
                    MessageType = Byte;
                    State = 1;
                    SizeBytesLeft = 0;

                    break;
            
                case 1: /* Have type, but no size */
                {
                    if(SizeBytesLeft > 0)
                    {
                        Buffer.Add <char> (Byte);
                        
                        if((-- SizeBytesLeft) == 0)
                        {
                            switch(Buffer.Size)
                            {
                            case 2:

                                MessageSize = *(lw_i16 *) Buffer.Buffer;
                                break;

                            case 4:

                                MessageSize = *(lw_i32 *) Buffer.Buffer;
                                break;
                            }

                            Buffer.Reset();

                            State = 3;
                            break;
                        }

                        break;
                    }

                    /* Byte is the first byte of the size */

                    if(Byte == 254)
                    {
                        /* 16 bit message size to follow */

                        SizeBytesLeft = 2;
                        break;
                    }

                    if(Byte == 255)
                    {
                        /* 32 bit message size to follow */

                        SizeBytesLeft = 4;
                        break;
                    }

                    /* 8 bit message size */

                    MessageSize = Byte;
                    State = 3;

                    break;
                }
            };
        }

        if(State < 3) /* Header not complete yet */
            return;

        if(Buffer.Size == 0)
        {
            if(Size == MessageSize)
            {
                /* The message isn't fragmented, and it's the only message. */

                MessageHandler(Tag, MessageType, Data, MessageSize);
                State = 0;

                return;
            }

            if(Size > MessageSize)
            {
                /* There message isn't fragmented, but there are more messages than
                   this one.  Lovely hack to give it a null terminator without copying
                   the message..!  */

                char NextByte = Data[MessageSize];
                Data[MessageSize] = 0;

                MessageHandler(Tag, MessageType, Data, MessageSize); 
                Data[MessageSize] = NextByte;

                State = 0;
                Process(Data + MessageSize, Size - MessageSize);

                return;
            }
        }

        unsigned int ThisMessageBytes = MessageSize - Buffer.Size;

        if(Size < ThisMessageBytes)
            ThisMessageBytes = Size;

        Buffer.Add(Data, ThisMessageBytes);
        
        Size -= ThisMessageBytes;
        Data += ThisMessageBytes;

        if(Buffer.Size == MessageSize)
        {
            Buffer.Add <char> (0);

            MessageHandler(Tag, MessageType, Buffer.Buffer, MessageSize);
            Buffer.Reset();

            State = 0;

            if(Size > 0)
                Process(Data, Size);
        }
    }


};

#endif

