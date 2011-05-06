
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

struct FilterInternal
{
    FilterInternal() : RemoteAddress((unsigned int) 0, 0)
    {
        LocalIP = 0;
        LocalPort = 0;
    }

    int LocalIP;
    int LocalPort;

    Lacewing::Address RemoteAddress;
};

Lacewing::Filter::Filter()
{
    InternalTag = new FilterInternal;
    Tag         = 0;
}

Lacewing::Filter::Filter(const Filter &_Filter)
{
    InternalTag = new FilterInternal;
    Tag         = 0;

    RemoteAddress (_Filter.RemoteAddress());
    LocalIP       (_Filter.LocalIP());
    LocalPort     (_Filter.LocalPort());
}

Lacewing::Filter::~Filter()
{
    delete ((FilterInternal *) InternalTag);
}

void Lacewing::Filter::RemoteAddress(Lacewing::Address &Address)
{
    while(!Address.Ready())
        LacewingYield();

    ((FilterInternal *) InternalTag)->RemoteAddress.~Address();
    new (&((FilterInternal *) InternalTag)->RemoteAddress) Lacewing::Address (Address);
}

void Lacewing::Filter::LocalIP(int IP)
{
    ((FilterInternal *) InternalTag)->LocalIP = IP;
}

int Lacewing::Filter::LocalIP() const
{
    return ((FilterInternal *) InternalTag)->LocalIP;
}

void Lacewing::Filter::LocalPort(int Port)
{
    ((FilterInternal *) InternalTag)->LocalPort = Port;
}

int Lacewing::Filter::LocalPort() const
{
    return ((FilterInternal *) InternalTag)->LocalPort;
}

Lacewing::Address &Lacewing::Filter::RemoteAddress() const
{
    return ((FilterInternal *) InternalTag)->RemoteAddress;
}

