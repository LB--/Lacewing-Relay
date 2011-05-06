
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

Lacewing::Event::Event()
{
    LacewingInitialise();

    Tag = 0;
    InternalTag = WSACreateEvent();
}

Lacewing::Event::~Event()
{
    WSACloseEvent((WSAEVENT) InternalTag);
}

bool Lacewing::Event::Signalled()
{
    return WSAWaitForMultipleEvents(1, (WSAEVENT *) &InternalTag, true, 0, false) == WAIT_OBJECT_0;
}

void Lacewing::Event::Signal()
{
    WSASetEvent((WSAEVENT) InternalTag);
}

void Lacewing::Event::Unsignal()
{
    WSAResetEvent((WSAEVENT) InternalTag);
}

void Lacewing::Event::Wait(int Timeout)
{
    WSAWaitForMultipleEvents(1, (WSAEVENT *) &InternalTag, true, Timeout == -1 ? INFINITE : Timeout, false);
}

