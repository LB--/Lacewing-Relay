
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

#ifdef HAVE_NETDB_H
    #include <netdb.h>
#endif

lw_iptr Ports [] =
{
    (lw_iptr) "http://",       80,
    (lw_iptr) "ftp://",        21,

    0
};

struct AddressInternal
{
    ThreadTracker Threads;

    char * Hostname;
    int Port;
    
    union
    {
        int IP;
        unsigned char Bytes[4];
    };

    AddressInternal(int Port)
    {
        this->Port = Port;
        *StringIP = 0;
        IP = 0;
    }

    ~AddressInternal()
    {
        Threads.WaitUntilDead();
    }

    char StringIP[32];

    void MakeStringIP()
    {
        if(!IP)
        {
            *StringIP = 0;
            return;
        }

        sprintf(StringIP, "%d.%d.%d.%d", Bytes[0], Bytes[1], Bytes[2], Bytes[3]);
    }

};

LacewingThread(Resolver, AddressInternal, Internal)
{
    hostent * Host;

    if(!(Host = gethostbyname(Internal.Hostname)))
    {
        Internal.IP = 0;
        
        free(Internal.Hostname);
        return;
    }

    Internal.IP = ((in_addr *) (Host->h_addr))->s_addr;
    Internal.MakeStringIP();
}

Lacewing::Address::Address()
{
    InternalTag = new AddressInternal(0);
    Tag = 0;

    AddressInternal &Internal = *(AddressInternal *) InternalTag;

    Internal.IP = 0;
    Internal.MakeStringIP();
}

Lacewing::Address::Address(const char * Hostname, int Port, bool Blocking)
{
    if(!*Hostname)
        Blocking = true;

    InternalTag = new AddressInternal(Port);
    Tag = 0;

    AddressInternal &Internal = *((AddressInternal *) InternalTag);

    char * Trimmed;
    Trim(Internal.Hostname = strdup(Hostname), Trimmed);

    if(!*Trimmed)
    {
        free(Internal.Hostname);
        return;
    }

    for(lw_iptr * i = Ports; *i; i += 2)
    {
        if(BeginsWith(Trimmed, (const char *) *i))
        {
            Internal.Port = i[1];
            Trimmed += strlen((const char *) *i);

            break;
        }
    }

    for(char * Iterator = Trimmed; *Iterator; ++ Iterator)
    {
        if(*Iterator != ':')
            continue;

        Internal.Port = atoi(Iterator + 1);
        *Iterator = 0;

        break;
    }

    if((Internal.IP = inet_addr(Trimmed)) != INADDR_NONE)
    {   
        DebugOut("inet_addr for " << Trimmed << " was valid " << Internal.IP);

        Internal.MakeStringIP();
        free(Internal.Hostname);
    }
    else
    {
        DebugOut("inet_addr for " << Trimmed << " was blank");

        Internal.IP = 0;

        if(Blocking)
            TResolver(*(AddressInternal *) InternalTag);
        else
            ((AddressInternal *) InternalTag)->Threads.Start((void *) Resolver, InternalTag);
    }
}

Lacewing::Address::Address(unsigned int IP, int Port)
{
    InternalTag = new AddressInternal(Port);
    Tag = 0;

    AddressInternal &Internal = *(AddressInternal *) InternalTag;

    Internal.IP = IP;
    Internal.MakeStringIP();
}

Lacewing::Address::Address(unsigned char IP_Byte1, unsigned char IP_Byte2,
                            unsigned char IP_Byte3, unsigned char IP_Byte4, int Port)
{
    InternalTag = new AddressInternal(Port);
    Tag = 0;

    AddressInternal &Internal = *(AddressInternal *) InternalTag;

    Internal.Bytes[0] = IP_Byte1;
    Internal.Bytes[1] = IP_Byte2;
    Internal.Bytes[2] = IP_Byte3;
    Internal.Bytes[3] = IP_Byte4;
    
    Internal.MakeStringIP();
}

Lacewing::Address::Address(const Lacewing::Address &Address)
{
    while(!Address.Ready())
        LacewingYield();

    InternalTag = new AddressInternal(Address.Port());
    Tag = 0;

    AddressInternal &Internal = *(AddressInternal *) InternalTag;

    Internal.IP = Address.IP();
    Internal.MakeStringIP();
}

Lacewing::Address::~Address()
{
    delete ((AddressInternal *) InternalTag);
}

int Lacewing::Address::Port() const
{
    if(!Ready())
        return -1;

    return ((AddressInternal *) InternalTag)->Port;
}

void Lacewing::Address::Port(int Port)
{
    if(!Ready())
        return;

    ((AddressInternal *) InternalTag)->Port = Port;
}

bool Lacewing::Address::Ready() const
{
    return !((AddressInternal *) InternalTag)->Threads.Living();
}

unsigned int Lacewing::Address::IP() const
{
    if(!Ready())
        return 0;

    return ((AddressInternal *) InternalTag)->IP;
}

unsigned char Lacewing::Address::IP_Byte(int Index) const
{
    if((!Ready()) || Index > 3 || Index < 0)
        return 0;

    return ((AddressInternal *) InternalTag)->Bytes[Index];
}

const char * Lacewing::Address::ToString() const
{
    if(!Ready())
        return "";

    return ((AddressInternal *) InternalTag)->StringIP;
}

Lacewing::Address::operator const char * () const
{
    return ToString();
}

