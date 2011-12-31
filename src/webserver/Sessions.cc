
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011 James McLaughlin.  All rights reserved.
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

#include "Common.h"

const char * const SessionCookie = "LacewingSession";

void Webserver::Request::Session (const char * Key, const char * Value)
{

    Webserver::Internal::Session * Session = internal->Server.FindSession (Cookie (SessionCookie));

    if (!Session)
    {
        char SessionID [128];

        sprintf (SessionID, "Session-%s-%d%d%d",
            internal->Client.Socket.GetAddress ().ToString (), (int) time (0),
                rand (), (int) (lw_iptr) this);

        MD5 (SessionID, SessionID);

        {   char SessionID_Hex [40];
            
            for(int i = 0; i < 16; ++ i)
                sprintf (SessionID_Hex + (i * 2), "%02x", ((unsigned char *) SessionID) [i]);

            Cookie (SessionCookie, SessionID_Hex);
        }

        Session = new Webserver::Internal::Session;

        Session->ID_Part1 = ((lw_i64 *) SessionID) [0];
        Session->ID_Part2 = ((lw_i64 *) SessionID) [1];

        if (internal->Server.FirstSession)
        {
            Session->Next = internal->Server.FirstSession;
            internal->Server.FirstSession = Session;
        }
        else
        {
            Session->Next = 0;
            internal->Server.FirstSession = Session;
        }
    }

    Session->Data.Set (Key, Value);
}

const char * Webserver::Request::Session (const char * Key)
{
    Webserver::Internal::Session * Session
        = internal->Server.FindSession (Cookie (SessionCookie));

    if (!Session)
        return "";

    return Session->Data.Get (Key);
}

void Webserver::CloseSession (const char * ID)
{
    Webserver::Internal::Session * Session = internal->FindSession (ID);

    if (Session == internal->FirstSession)
        internal->FirstSession = Session->Next;
    else
    {
        for (Webserver::Internal::Session * S = internal->FirstSession; S; S = S->Next)
        {
            if (S->Next == Session)
            {
                S->Next = Session->Next;
                break;
            }
        }
    }

    delete Session;
}

void Webserver::Request::CloseSession ()
{
    internal->Server.Webserver.CloseSession (Session ());
}

const char * Webserver::Request::Session ()
{
    return Cookie (SessionCookie);
}

Webserver::Internal::Session * Webserver::Internal::FindSession (const char * SessionID_Hex)
{
    if (strlen (SessionID_Hex) != 32)
        return 0;

    union
    {
        char SessionID_Bytes [16];
        
        struct
        {
            lw_i64 Part1;
            lw_i64 Part2;

        } SessionID;
    };

    char hex [3];
    hex [2] = 0;

    for (int i = 0, c = 0; i < 16; ++ i, c += 2)
    {
        hex [0] = SessionID_Hex [c];
        hex [1] = SessionID_Hex [c + 1];

        SessionID_Bytes [i] = (char) strtol (hex, 0, 16);
    }

    Webserver::Internal::Session * Session;

    for (Session = FirstSession; Session; Session = Session->Next)
    {
        if (Session->ID_Part1 == SessionID.Part1 &&
                Session->ID_Part2 == SessionID.Part2)
        {
            break;
        }
    }

    return Session;
}


Webserver::Request::SessionItem * Webserver::Request::FirstSessionItem ()
{

    Webserver::Internal::Session * Session = internal->Server.FindSession (Cookie (SessionCookie));

    if (!Session)
        return 0;

    return (Webserver::Request::SessionItem *) Session->Data.First;
}

Webserver::Request::SessionItem *
        Webserver::Request::SessionItem::Next ()
{
    return (Webserver::Request::SessionItem *) ((Map::Item *) this)->Next;
}

const char * Webserver::Request::SessionItem::Name ()
{
    return ((Map::Item *) this)->Key;
}

const char * Webserver::Request::SessionItem::Value ()
{
    return ((Map::Item *) this)->Value;
}

