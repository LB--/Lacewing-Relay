
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

class MD5Hasher
{

protected:

    #ifdef LacewingWindows
        HCRYPTPROV Context;
    #endif

public:

    MD5Hasher();
    ~MD5Hasher();

    void Hash(char * Buffer, unsigned int Size);
    
    inline void HashBase64(char * Buffer, unsigned int Size)
    {
        Hash(Buffer, Size);

        char Base64[40];
        
        for(int i = 0; i < 16; ++ i)
            sprintf(Base64 + (i * 2), "%02x", ((unsigned char *) Buffer) [i]);

        strcpy(Buffer, Base64);
    }
    
};

