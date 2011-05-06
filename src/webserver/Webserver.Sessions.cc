
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

#include "Webserver.Common.h"

const char * const SessionCookie = "LacewingSession";


void Lacewing::Webserver::Request::Session(const char * Key, const char * Value)
{
    WebserverClient &Internal = *((WebserverClient *) InternalTag);

    if(!*Session("LacewingValidSession"))
    {
        char Session[128];

        sprintf(Session, "Session-%s-%d%d%d%d%d",
            Internal.Socket.GetAddress().ToString(), (int) time(0),
                    rand(), rand(), rand(), (int) (lw_iptr) this);

        Internal.Server.MD5.HashBase64(Session, strlen(Session));
        
        Cookie(SessionCookie, Session);
    }

    const char * Session = Cookie(SessionCookie);

    Lacewing::SpinSync::WriteLock Lock(Internal.Server.Sync_Sessions);
        
        map<string, Map> &Sessions = Internal.Server.Sessions;
        map<string, Map>::iterator Found = Sessions.find(Session);

        if(Found == Sessions.end())
        {
            Sessions[Session].Set("LacewingValidSession", "1");
            Found = Sessions.find(Session);
        }

        Found->second.Set(Key, Value);
}

const char * Lacewing::Webserver::Request::Session(const char * Key)
{
    WebserverClient &Internal = *((WebserverClient *) InternalTag);
 
    const char * Session = Cookie(SessionCookie);

    if(!*Session)
        return "";

    Lacewing::SpinSync::ReadLock Lock(Internal.Server.Sync_Sessions);
        
        map<string, Map> &Sessions = Internal.Server.Sessions;
        map<string, Map>::iterator Found = Sessions.find(Session);

        if(Found == Sessions.end())
            return "";

        const char * Result = Found->second.Get(Key);

        if(!*Result)
            return "";


        /* Using a Map to create request-local copies, since the session Map is shared among
           threads (so the returned value isn't safe once Sync_Sessions is unlocked). */

        const char * CurrentCopy = Internal.SessionDataCopies.Get(Key);

        if(!strcmp(CurrentCopy, Result))
            return CurrentCopy;

        return Internal.SessionDataCopies.Set(Key, Result);
}

void Lacewing::Webserver::CloseSession(const char * Session)
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);

    Lacewing::SpinSync::WriteLock Lock(Internal.Sync_Sessions);
        
        map<string, Map>::iterator Found = Internal.Sessions.find(Session);

        if(Found != Internal.Sessions.end())
            Internal.Sessions.erase(Found);
}

void Lacewing::Webserver::Request::CloseSession()
{
    WebserverClient &Internal = *((WebserverClient *) InternalTag);
    Internal.Server.Webserver.CloseSession(Session());
}

const char * Lacewing::Webserver::Request::Session()
{
    return Cookie(SessionCookie);
}


