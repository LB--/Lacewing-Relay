
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

/* Internal */

WebserverClient::WebserverClient(Lacewing::Server::Client &_Socket)
    : Socket(_Socket), Server(*(WebserverInternal *) _Socket.Tag), Input(*this),
        Output(*(WebserverInternal *) _Socket.Tag, *this)
{
    Request.InternalTag   = this;
    Request.Tag           = 0;

    ConnectHandlerCalled  = false;
    RequestUnfinished     = false;
    Disconnected          = false;
}

char * WebserverInternal::BorrowSendBuffer()
{
    Lacewing::Sync::Lock Lock(Sync_SendBuffers);

    if(SendBuffers.empty())
        for(int i = WebserverInternal::SendBufferBacklog; i; -- i)
            SendBuffers.push_back(new char[WebserverInternal::SendBufferSize]);

    char * Back = SendBuffers.back();
    SendBuffers.pop_back();
    return Back;
}

void WebserverInternal::ReturnSendBuffer(char * SendBuffer)
{
    Lacewing::Sync::Lock Lock(Sync_SendBuffers);
    SendBuffers.push_back(SendBuffer);
}

void WebserverInternal::SocketConnect(Lacewing::Server &Server, Lacewing::Server::Client &Client)
{
    Client.Tag = Server.Tag;
    Client.Tag = &((WebserverInternal *) Server.Tag)->ClientBacklog.Borrow(Client);

    ((WebserverClient *) Client.Tag)->Secure =
        (&Server == ((WebserverInternal *) Server.Tag)->SecureSocket);
}

void WebserverInternal::SocketDisconnect(Lacewing::Server &Server, Lacewing::Server::Client &Client)
{
    WebserverInternal &Webserver = *(WebserverInternal *) Server.Tag;
    WebserverClient &WebClient = *(WebserverClient *) Client.Tag;

    WebClient.Disconnected = true;

    if(WebClient.ConnectHandlerCalled)
    {
        if(Webserver.HandlerDisconnect)
            Webserver.HandlerDisconnect(Webserver.Webserver, WebClient.Request);
    }
    else
    {
        /* TODO - Return the request struct */
    }
}

void WebserverInternal::SocketReceive(Lacewing::Server &Server, Lacewing::Server::Client &Client, char * Buffer, int Size)
{
    WebserverClient &WebClient = *(WebserverClient *) Client.Tag;

    WebClient.Input.Process(Buffer, Size);
}

void WebserverInternal::SocketError(Lacewing::Server &Server, Lacewing::Error &Error)
{
    Error.Add("Socket error");

    WebserverInternal &Webserver = *(WebserverInternal *) Server.Tag;

    if(Webserver.HandlerError)
        Webserver.HandlerError(Webserver.Webserver, Error);
}


/* Public - constructor/destructor */

Lacewing::Webserver::Webserver(Lacewing::Pump &EventPump)
{
    LacewingInitialise();

    InternalTag = new WebserverInternal(*this, *(PumpInternal *) EventPump.InternalTag);
    Tag         = 0;
}

Lacewing::Webserver::~Webserver()
{
    Unhost();
    UnhostSecure();

    delete ((WebserverInternal *) InternalTag);
}



/* Public - hosting/unhosting */

void Lacewing::Webserver::Host(int Port)
{
    Lacewing::Filter Filter;
    Filter.LocalPort(Port);

    Host(Filter);
}

void Lacewing::Webserver::Host(Lacewing::Filter &Filter)
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);
    Internal.PrepareSocket();

    if(!Filter.LocalPort())
        Filter.LocalPort(80);

    Internal.Socket->Host(Filter, true);
}

void Lacewing::Webserver::HostSecure(int Port)
{
    Lacewing::Filter Filter;
    Filter.LocalPort(Port);

    HostSecure(Filter);
}

void Lacewing::Webserver::HostSecure(Lacewing::Filter &Filter)
{
    if(!CertificateLoaded())
        return;

    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);
    Internal.PrepareSecureSocket();

    if(!Filter.LocalPort())
        Filter.LocalPort(443);

    Internal.SecureSocket->Host(Filter, true);
}

void Lacewing::Webserver::Unhost()
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);

    if(Internal.Socket)
        Internal.Socket->Unhost();
}

void Lacewing::Webserver::UnhostSecure()
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);

    if(Internal.SecureSocket)
        Internal.SecureSocket->Unhost();
}

bool Lacewing::Webserver::Hosting()
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);

    if(!Internal.Socket)
        return false;

    return Internal.Socket->Hosting();
}

bool Lacewing::Webserver::HostingSecure()
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);

    if(!Internal.SecureSocket)
        return false;

    return Internal.SecureSocket->Hosting();
}

int Lacewing::Webserver::Port()
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);

    if(!Internal.Socket)
        return 0;

    return Internal.Socket->Port();
}

int Lacewing::Webserver::SecurePort()
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);

    if(!Internal.SecureSocket)
        return 0;

    return Internal.SecureSocket->Port();
}

lw_i64 Lacewing::Webserver::BytesSent()
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);

    return (Internal.Socket ? Internal.Socket->BytesSent() : 0)
        + (Internal.SecureSocket ? Internal.SecureSocket->BytesSent() : 0);
}

lw_i64 Lacewing::Webserver::BytesReceived()
{
   WebserverInternal &Internal = *((WebserverInternal *) InternalTag);

    return (Internal.Socket ? Internal.Socket->BytesReceived() : 0)
        + (Internal.SecureSocket ? Internal.SecureSocket->BytesReceived() : 0);
}


/* Public - certificate management */

bool Lacewing::Webserver::LoadCertificateFile(const char * Filename, const char * CommonName)
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);
    Internal.PrepareSecureSocket();

    return Internal.SecureSocket->LoadCertificateFile(Filename, CommonName);
}

bool Lacewing::Webserver::LoadSystemCertificate(const char * StoreName, const char * CommonName, const char * Location)
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);
    Internal.PrepareSecureSocket();

    return Internal.SecureSocket->LoadSystemCertificate(StoreName, CommonName, Location);
}

bool Lacewing::Webserver::CertificateLoaded()
{
    WebserverInternal &Internal = *((WebserverInternal *) InternalTag);
    Internal.PrepareSecureSocket();

    return Internal.SecureSocket->CertificateLoaded();
}


/* Public - Request */

void Lacewing::Webserver::EnableManualRequestFinish()
{
    ((WebserverInternal *) InternalTag)->AutoFinish = false;
}

Lacewing::Address &Lacewing::Webserver::Request::GetAddress()
{
    return ((WebserverClient *) InternalTag)->Socket.GetAddress();
}

void Lacewing::Webserver::Request::Disconnect()
{
    ((WebserverClient *) InternalTag)->Socket.Disconnect();
}

void Lacewing::Webserver::Request::Send(const char * Data, int Size)
{
    ((WebserverClient *) InternalTag)->Output.AddSend(Data, Size);
}

void Lacewing::Webserver::Request::SendConstant(const char * Data, int Size)
{
    if(Size == -1)
        Size = strlen(Data);

    char SendDesc[sizeof(void *) * 3];

    *(unsigned char *)(SendDesc)     = 0xFF;
    *(const char **)  (SendDesc + 1) = Data;

    *(unsigned int *)  
        (SendDesc + 1 + sizeof(unsigned char *)) = Size;

    ((WebserverClient *) InternalTag)->Output.AddFileSend(SendDesc, sizeof(SendDesc), Size);
}

bool Lacewing::Webserver::Request::SendFile(const char * Filename)
{
    lw_i64 ThisFileSize = Lacewing::FileSize(Filename);

    if(!ThisFileSize)
        return false;

    ((WebserverClient *) InternalTag)->Output.AddFileSend(Filename, strlen(Filename) + 1, ThisFileSize);

    return true;
}

void Lacewing::Webserver::Request::Reset()
{
    WebserverClient::Outgoing &Output = ((WebserverClient *) InternalTag)->Output;

    for(list<char *>::iterator it = Output.SendBuffers.begin(); it != Output.SendBuffers.end(); ++ it)
        Output.Server.ReturnSendBuffer(*it);
    
    Output.LastSendBufferSize = 0;
    
    WebserverClient::Outgoing::File * File = Output.FirstFile;

    while(File)
    {
        delete File;
        File = File->Next;
    }

    Output.FirstFile = 0;
    Output.TotalFileSize = 0;
}

void Lacewing::Webserver::Request::GuessMimeType(const char * Filename)
{
    SetMimeType(Lacewing::GuessMimeType(Filename));
}

void Lacewing::Webserver::Request::SetMimeType(const char * MimeType)
{
    strcpy(((WebserverClient *) InternalTag)->Output.MimeType, MimeType);
}

void Lacewing::Webserver::Request::SetRedirect(const char * URL)
{
    SetResponseType(303, "See Other");
    Header("Location", URL);
}

void Lacewing::Webserver::Request::DisableCache()
{
    Header("Cache-Control", "no-cache");
}

void Lacewing::Webserver::Request::SetCharset(const char * Charset)
{
    strcpy(((WebserverClient *) InternalTag)->Output.Charset, Charset);
}

void Lacewing::Webserver::Request::Header(const char * Name, const char * Value)
{
    ((WebserverClient *) InternalTag)->Output.Headers.Set(Name, Value);
}

void Lacewing::Webserver::Request::Cookie(const char * Name, const char * Value)
{
    ((WebserverClient *) InternalTag)->Cookies.Set(Name, Value);
}

void Lacewing::Webserver::Request::SetResponseType(int StatusCode, const char * Message)
{
    sprintf(((WebserverClient *) InternalTag)->Output.ResponseCode, "%d %s", StatusCode, Message);
}

void Lacewing::Webserver::Request::SetUnmodified()
{
    SetResponseType(304, "Not Modified");
}

void Lacewing::Webserver::Request::LastModified(lw_i64 Time)
{
    tm TM;

    time_t TimeT = (time_t) Time;
    gmtime_r(&TimeT, &TM);

    char LastModified[128];
    sprintf(LastModified, "%s, %02d %s %d %02d:%02d:%02d GMT", Weekdays[TM.tm_wday], TM.tm_mday,
                            Months[TM.tm_mon], TM.tm_year + 1900, TM.tm_hour, TM.tm_min, TM.tm_sec);

    Header("Last-Modified", LastModified);
}

void Lacewing::Webserver::Request::Finish(const char * Data, int Size)
{
    WebserverClient &Client = *((WebserverClient *) InternalTag);


    /* For a disconnected client, Finish() is either called in the disconnect handler or
       afterwards.  After the disconnect handler has been called, Lacewing::Server won't
       call any more handlers, so there's no sync issue with the Disconnected bool. */

    if(Client.Disconnected)
    {
        DebugOut("Finish: Was DC'd");

        Client.Server.ClientBacklog.Return(Client);
        return;
    }

    if(!Client.RequestUnfinished)
    {
        LacewingAssert(false);

        DebugOut("Finish: Request not marked as unfinished");
        return;
    }

    if(Data && *Data)
        SendConstant(Data, Size == -1 ? strlen(Data) : Size);

    ((WebserverClient *) InternalTag)->Output.Respond();
    
    Client.RequestUnfinished = false;

    /* TODO process what's been buffered */
}

const char * Lacewing::Webserver::Request::Header(const char * Name)
{
    return ((WebserverClient *) InternalTag)->Input.Headers.Get(Name);
}

const char * Lacewing::Webserver::Request::Cookie(const char * Name)
{
    return ((WebserverClient *) InternalTag)->Cookies.Get(Name);
}

const char * Lacewing::Webserver::Request::GET(const char * Name)
{
    return ((WebserverClient *) InternalTag)->Input.GetItems.Get(Name);
}

const char * Lacewing::Webserver::Request::POST(const char * Name)
{
    return ((WebserverClient *) InternalTag)->Input.PostItems.Get(Name);
}

lw_i64 Lacewing::Webserver::Request::LastModified()
{
    const char * LastModified = Header("If-Modified-Since");

    if(*LastModified)
        return ParseTimeString(LastModified);

    return 0;
}

bool Lacewing::Webserver::Request::Secure()
{
    return ((WebserverClient *) InternalTag)->Secure;
}

const char * Lacewing::Webserver::Request::Hostname()
{
    return ((WebserverClient *) InternalTag)->Input.Hostname;
}

const char * Lacewing::Webserver::Request::URL()
{
    return ((WebserverClient *) InternalTag)->Input.URL;
}


AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, Get)
AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, Post)
AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, Head)
AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, Error)
AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, Connect)
AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, Disconnect)
AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, UploadStart)
AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, UploadChunk)
AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, UploadDone)
AutoHandlerFunctions(Lacewing::Webserver, WebserverInternal, UploadPost)


