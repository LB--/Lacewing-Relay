
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

struct ErrorInternal
{
    char Error [4096];
    char * Begin;

    ErrorInternal()
    {
        *(Begin = Error + sizeof(Error) - 1) = 0;
    }

    inline void Add(const char * Text)
    {
        int Length = strlen(Text);
        memcpy(Begin -= Length, Text, Length);
    }
};

Lacewing::Error::Error()
{
    InternalTag = new ErrorInternal;
}

Lacewing::Error::~Error()
{
    if(!InternalTag)
        return;

    delete ((ErrorInternal *) InternalTag);
}

Lacewing::Error::operator const char *()
{
    return ToString();
}

const char * Lacewing::Error::ToString()
{
    return ((ErrorInternal *) InternalTag)->Begin;
}

Lacewing::Error * Lacewing::Error::Clone()
{
    ErrorInternal &Internal = *((ErrorInternal *) InternalTag);

    Lacewing::Error * New = new Lacewing::Error;
    New->Add ("%s", ToString());

    return New;
}

void Lacewing::Error::Add(const char * Format, ...)
{
    ErrorInternal &Internal = *((ErrorInternal *) InternalTag);

    char Buffer[2048];

    va_list Arguments;
    va_start (Arguments, Format);
    
    vsnprintf(Buffer, sizeof(Buffer), Format, Arguments);
    
    va_end (Arguments);

    if(*Internal.Begin)
        Internal.Add(" - ");

    Internal.Add(Buffer);
}

void Lacewing::Error::Add(int Error)
{
    #ifdef LacewingWindows

        char * Message;

        if(FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            0, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *) &Message, 1, 0))
        {
            Add(Error < 0 ? "%s (%08X)" : "%s (%d)", Message, Error);
        }

        LocalFree(Message);

    #else

        Add(strerror(Error));
        
    #endif
}


