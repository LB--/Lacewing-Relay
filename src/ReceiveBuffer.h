
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

class ReceiveBuffer
{
protected:

    size_t NextSize, MaxSize;

public:

    char * Buffer;
    size_t Size;

    inline ReceiveBuffer(int MaxKB = 1024)
    {
        Buffer   = 0;
        Size     = 0;
        NextSize = (MaxSize = MaxKB * 1024) / 8;
    }

    inline ~ReceiveBuffer()
    {
        free(Buffer);
    }
    
    inline operator char * ()
    {
        return Buffer;
    }

    inline void Prepare()
    {
        if(NextSize != 0)
        {
            char * NewBuffer = (char *) malloc(NextSize + 1);

            if(NewBuffer)
            {
                free (Buffer);

                Buffer = NewBuffer;    
                Size   = NextSize;

                NextSize = 0;
            }
            else
            {
                /* Cancel the upgrade, since malloc failing can't be a good sign */

                NextSize = 0;
            }
        }
    }

    inline void Received(size_t Size)
    {
        Buffer[Size] = 0;

        if(Size == this->Size && this->Size < MaxSize)
            NextSize = this->Size * 2;
    }

    /* TODO: A way for the buffer to be downsized again when the capacity
       isn't being reached anymore? */

};

