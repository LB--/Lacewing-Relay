
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

struct Address::Internal
{
    Thread ResolverThread;

    Internal ();
    ~ Internal ();

    void Init (const char * Hostname, const char * Service, int Hints);

    char * Hostname, * HostnameToFree;
    char Service [64]; /* port or service name */

    int Hints;

    addrinfo * InfoList, * Info, * InfoToFree;

    Lacewing::Error * Error;

    inline void SetError (Lacewing::Error * Error)
    {
        delete this->Error;
        this->Error = Error;
    }

    char Buffer [64];
    const char * ToString ();
};


/* Used internally to wrap a sockaddr into an Address, without any dynamic memory allocation */

struct AddressWrapper : public Address::Internal
{
    Lacewing::Address * Address;
    char AddressBytes [sizeof (Lacewing::Address)];

    addrinfo info;

    inline AddressWrapper ()
    {
        Address = (Lacewing::Address *) AddressBytes;

        Address->Tag = 0;
        Address->internal = this;

        memset (&info, 0, sizeof (info));

        Info = &info;
    }

    inline void Set (sockaddr_storage * s)
    {
        info.ai_addr = (sockaddr *) s;
        
        if ((info.ai_family = s->ss_family) == AF_INET6)
            info.ai_addrlen = sizeof (sockaddr_in6);
        else
            info.ai_addrlen = sizeof (sockaddr_in);
    }

    inline operator Lacewing::Address & ()
    {
        return *Address;
    }
};

