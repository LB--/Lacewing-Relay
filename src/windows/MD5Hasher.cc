
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

#include "../Common.h"

MD5Hasher::MD5Hasher()
{
    CryptAcquireContext(&Context, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
}

MD5Hasher::~MD5Hasher()
{
    CryptReleaseContext(Context, 0);
}

void MD5Hasher::Hash(char * Buffer, unsigned int Size)
{
    HCRYPTPROV HashProvider;

    CryptCreateHash(Context, CALG_MD5, 0, 0, &HashProvider);
    CryptHashData(HashProvider, (BYTE *) Buffer, Size, 0);

    DWORD HashLength = 16;
    CryptGetHashParam(HashProvider, HP_HASHVAL, (BYTE *) Buffer, &HashLength, 0);

    CryptDestroyHash(HashProvider);
}

