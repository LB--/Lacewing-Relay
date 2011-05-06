
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

#ifndef LacewingMessageReader
#define LacewingMessageReader

class MessageReader
{
protected:

    char * Buffer;
    
    unsigned int Size;
    unsigned int Offset;

    list<char *> ToFree;

public:

    bool Failed;

    inline MessageReader(char * Buffer, unsigned int Size)
    {
        Failed = false;

        this->Buffer = Buffer;
        this->Size = Size;

        this->Offset = 0;
    }

    inline ~MessageReader()
    {
        for(list<char *>::iterator it = ToFree.begin(); it != ToFree.end(); ++ it)
            delete [] *it;
    }

    inline bool Check(unsigned int Size)
    {
        if(Failed)
            return false;

        if(Offset + Size > this->Size)
        {
            Failed = true;
            return false;
        }

        return true;
    }

    template<class T> inline T Get()
    {
        if(!Check(sizeof(T)))
            return 0;

        T Value = *(T *) (Buffer + Offset);

        Offset += sizeof(T);
        return Value;
    }

    char * Get (unsigned int Size)
    {
        if(!Check(Size))
            return 0;

        char * Output = (char *) malloc (Size + 1);

        if(!Output)
        {
            Failed = true;
            return 0;
        }

        ToFree.push_back(Output);

        memcpy(Output, Buffer + Offset, Size);
        Output[Size] = 0;

        Offset += Size;

        return Output;
    }


    inline char * GetRemaining(bool AllowEmpty = true)
    {
        if(Failed)
            return this->Buffer;

        char * Remaining = this->Buffer + Offset;
        Offset += Size;

        if(!AllowEmpty && !*Remaining)
            Failed = true;

        return Remaining;
    }

    inline void GetRemaining(char * &Buffer, unsigned int &Size, unsigned int MinimumLength = 0, unsigned int MaximumLength = 0xFFFFFFFF)
    {
        Buffer = this->Buffer + Offset;
        Size   = this->Size - Offset;

        if(Size > MaximumLength || Size < MinimumLength)
            Failed = true;

        Offset += Size;
    }

};

#endif

