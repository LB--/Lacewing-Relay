
/*
 * Copyright (c) 2011 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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


