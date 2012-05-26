
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
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

static void Resolver (Address::Internal *);

Address::Internal::Internal () : ResolverThread ("Resolver", (void *) ::Resolver)
{
    Error = 0;
    *Buffer = 0;
    
    Hostname = HostnameToFree = 0;
    Info = InfoList = InfoToFree = 0;
}

Address::Internal::~ Internal ()
{
    ResolverThread.Join ();

    free (HostnameToFree);

    delete Error;

    if (InfoList)
    {
        #ifdef LacewingWindows
            Compat::fn_freeaddrinfo freeaddrinfo = Compat::freeaddrinfo ();
        #endif

        freeaddrinfo (InfoList);
    }

    if (InfoToFree)
    {
        free (InfoToFree->ai_addr);
        free (InfoToFree);
    }
}

const char * Address::Internal::ToString ()
{
    if (*Buffer)
        return Buffer;

    if ((!Info) || (!Info->ai_addr))
        return "";

    switch (Info->ai_family)
    {
        case AF_INET:

            lw_snprintf (Buffer, sizeof (Buffer), "%s:%d",
                                inet_ntoa (((sockaddr_in *) Info->ai_addr)->sin_addr),
                                        ntohs (((sockaddr_in *) Info->ai_addr)->sin_port));

            break;

        case AF_INET6:
        {             
            int Length = sizeof (Buffer) - 1;

            #ifdef LacewingWindows

                WSAAddressToStringA
                    ((LPSOCKADDR) Info->ai_addr, (DWORD) Info->ai_addrlen, 0, Buffer,
                            (LPDWORD) &Length);

            #else

                inet_ntop
                    (AF_INET6, &((sockaddr_in6 *) Info->ai_addr)->sin6_addr, Buffer, Length);

            #endif

            lw_snprintf (Buffer + strlen (Buffer) - 1,
                sizeof (Buffer) - strlen (Buffer), ":%d",
                    ntohs (((sockaddr_in6 *) Info->ai_addr)->sin6_port));

            break;
        }
    };

    return Buffer ? Buffer: "";
}

void Resolver (Address::Internal * internal)
{
    addrinfo Hints;
    memset (&Hints, 0, sizeof (Hints));

    if (internal->Hints & Address::HINT_TCP)
    {
        if (!(internal->Hints & Address::HINT_UDP))
            Hints.ai_socktype = SOCK_STREAM;
    }
    else
    {
        if (internal->Hints & Address::HINT_UDP)
            Hints.ai_socktype = SOCK_DGRAM;
    }

    Hints.ai_protocol  =  0;
    Hints.ai_flags     =  AI_V4MAPPED | AI_ADDRCONFIG;

    if (internal->Hints & Address::HINT_IPv4)
        Hints.ai_family = AF_INET;
    else
        Hints.ai_family = AF_INET6;

    #ifdef LacewingWindows
        Compat::fn_getaddrinfo getaddrinfo = Compat::getaddrinfo ();
    #endif

    int result = getaddrinfo
        (internal->Hostname, internal->Service, &Hints, &internal->InfoList);

    if (result != 0)
    {
        Lacewing::Error * Error = new Lacewing::Error;

        Error->Add ("%s", gai_strerror (result));
        Error->Add ("getaddrinfo error");

        internal->SetError (Error);

        return;
    }

    for (addrinfo * Info = internal->InfoList; Info; Info = Info->ai_next)
    {
        if (Info->ai_family == AF_INET6)
        {
            internal->Info = Info;
            break;
        }

        if (Info->ai_family == AF_INET)
        {
            internal->Info = Info;
            break;
        }
    }
}

Address::Address (const char * Hostname, int Port, int Hints)
{
    internal = new Internal;

    Tag = 0;

    char Service [64];
    lw_snprintf (Service, sizeof (Service), "%d", Port);

    internal->Init (Hostname, Service, Hints);
}

Address::Address (const char * Hostname, const char * Service, int Hints)
{
    internal = new Internal;

    Tag = 0;

    internal->Init (Hostname, Service, Hints);
}

void Address::Internal::Init (const char * _Hostname, const char * Service, int Hints)
{
    this->Hints = Hints;

    HostnameToFree = Hostname = Trim (_Hostname);

    for (char * it = Hostname; *it; ++ it)
    {
        if (it [0] == ':' && it [1] == '/' && it [2] == '/')
        {
            *it = 0;

            Service = Hostname;
            Hostname = it + 3;
        }

        if (*it == ':')
        {
            /* An explicit port overrides the service name */

            Service = it + 1;
            *it = 0;
        }
    }

    CopyString (this->Service, Service, sizeof (this->Service)); 

    ResolverThread.Join (); /* block if the thread is already running */
    ResolverThread.Start (this);
}

Address::Address (Address &Address)
{
    internal = new Internal;

    Tag = 0;

    {   Error * Error = Address.Resolve ();

        if (Error)
        {
            internal->SetError (Error->Clone ());
            return;
        }
    }

    addrinfo * info = Address.internal->Info;

    if (!info)
        return;

    addrinfo * new_info = (addrinfo *) malloc (sizeof (addrinfo));
    memcpy (new_info, info, sizeof (addrinfo));

    new_info->ai_next = 0;
    new_info->ai_addr = (sockaddr *) malloc (info->ai_addrlen);

    memcpy (new_info->ai_addr, info->ai_addr, info->ai_addrlen);

    internal->Info = internal->InfoToFree = new_info;
}

Address::~Address ()
{
    delete internal;
}

bool Address::Ready ()
{
    return !internal->ResolverThread.Started ();
}

const char * Address::ToString ()
{
    if (!Ready ())
        return "";

    return internal->ToString ();
}

Address::operator const char * ()
{
    return ToString ();
}

int Address::Port ()
{
    if ((!Ready ()) || !internal->Info || !internal->Info->ai_addr)
        return 0;

    return ntohs (internal->Info->ai_family == AF_INET6 ?
        ((sockaddr_in6 *) internal->Info->ai_addr)->sin6_port :
            ((sockaddr_in *) internal->Info->ai_addr)->sin_port);
}

void Address::Port (int Port)
{
    if ((!Ready ()) || !internal->Info || !internal->Info->ai_addr)
        return;

    *internal->Buffer = 0;

    if (internal->Info->ai_family == AF_INET6)
        ((sockaddr_in6 *) internal->Info->ai_addr)->sin6_port = htons (Port);
    else
        ((sockaddr_in *) internal->Info->ai_addr)->sin_port = htons (Port);
}

Error * Address::Resolve ()
{
    internal->ResolverThread.Join ();

    return internal->Error;
}

static bool SockaddrEqual (sockaddr * a, sockaddr * b)
{
    if ((!a) || (!b))
        return false;

    if (a->sa_family == AF_INET6)
    {
        if (b->sa_family != AF_INET6)
            return false;
        
        return !memcmp (&((sockaddr_in6 *) a)->sin6_addr,
                        &((sockaddr_in6 *) b)->sin6_addr,
                        sizeof (in6_addr));
    }

    if (a->sa_family == AF_INET)
    {
        if (b->sa_family != AF_INET)
            return false;
        
        return !memcmp (&((sockaddr_in *) a)->sin_addr,
                        &((sockaddr_in *) b)->sin_addr,
                        sizeof (in6_addr));
    }

    return false;
}

bool Address::operator == (Address &Address)
{
    if ((!internal->Info) || (!Address.internal->Info))
        return true;

    return SockaddrEqual
        (internal->Info->ai_addr, Address.internal->Info->ai_addr);
}

bool Address::operator != (Address &Address)
{
    if ((!internal->Info) || (!Address.internal->Info))
        return true;

    return !SockaddrEqual
        (internal->Info->ai_addr, Address.internal->Info->ai_addr);
}

bool Address::IPv6 ()
{
    return internal->Info &&
        internal->Info->ai_addr &&
        ((sockaddr_storage *) internal->Info->ai_addr)->ss_family == AF_INET6;
}

