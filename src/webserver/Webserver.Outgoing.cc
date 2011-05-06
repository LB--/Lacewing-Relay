
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

WebserverClient::Outgoing::Outgoing(WebserverInternal &_Server, WebserverClient &_Client)
    : Server(_Server), Client(_Client)
{
    FirstFile = 0;

    Reset();
}

void WebserverClient::Outgoing::RunHandler()
{
    Lacewing::Server::Client &Socket = Client.Socket;

    if(Client.Secure)
    {
        /* When the request is secure, add the "Cache-Control: public" header
           by default.  DisableCache() will remove this, and should be used for
           any pages containing sensitive data that shouldn't be cached.  */

        Headers.Set("Cache-Control", "public");
    }

    if(!Client.ConnectHandlerCalled)
    {
        if(Server.HandlerConnect)
            Server.HandlerConnect(Server.Webserver, Client.Request);

        Client.ConnectHandlerCalled = true;
    }

    do
    {
        if(!strcmp(Client.Input.Method, "GET"))
        {
            if(Server.HandlerGet)
                Server.HandlerGet(Server.Webserver, Client.Request);

            break;
        }

        if(!strcmp(Client.Input.Method, "POST"))
        {
            if(!Client.Input.BodyProcessor)
            {
                if(Server.HandlerPost)
                    Server.HandlerPost(Server.Webserver, Client.Request);
            }
            else
                Client.Input.BodyProcessor->CallRequestHandler();

            break;
        }

        if(!strcmp(Client.Input.Method, "HEAD"))
        {
            if(Server.HandlerHead)
                Server.HandlerHead(Server.Webserver, Client.Request);

            break;
        }

        Socket << Client.Input.Version << " 501 Not Implemented\r\n\r\n";
        return;

    } while(0);

    if(Client.RequestUnfinished && Server.AutoFinish)
        Client.Request.Finish();
}

void WebserverClient::Outgoing::Respond()
{
    Lacewing::Server::Client &Socket = Client.Socket;

    /* Response line and headers */

    Socket.StartBuffering();

    Socket << Client.Input.Version << " " << ResponseCode;
    Socket << "\r\nContent-Type: " << MimeType;
    
    if(*Charset)
        Socket << "; charset=" << Charset;

    Socket << "\r\nServer: " << Lacewing::Version();

    for(Map::Item * Current = Headers.First; Current; Current = Current->Next)
        Socket << "\r\n" << Current->Key << ": " << Current->Value;

    for(Map::Item * Current = Client.Cookies.First; Current; Current = Current->Next)
        if(strcmp(Current->Value, Client.Input.Cookies.Get(Current->Key)))
            Socket << "\r\nSet-Cookie: " << Current->Key << "=" << Current->Value;

    lw_i64 ContentLength = TotalFileSize;

    if(SendBuffers.size())
        ContentLength += (WebserverInternal::SendBufferSize * (SendBuffers.size() - 1)) + LastSendBufferSize;

    Socket << "\r\nContent-Length: " << ContentLength;
    Socket << "\r\n\r\n";


    /* SendFile flushes the output (and can do so as part of the TransmitFile call on windows)
       If there aren't any files, we need to flush manually now so page body isn't buffered.
       
       01/2011: Now allows small pages (eg AJAX responses) to be buffered, so the entire response
       can be sent at once. */

    if(!FirstFile)
    {
        if(ContentLength > (1024 * 8)) /* Pages over 8 KB will not be buffered */
            Socket.Flush();
    }

    /* Response body */

    while(FirstFile && FirstFile->Position <= 0 && FirstFile->Offset == 0)
    {
        FirstFile->Send(Socket);
        
        WebserverClient::Outgoing::File * Next = FirstFile->Next;
        delete FirstFile;
        FirstFile = Next;
    }

    int SendBufferIndex = 0;
    list<char *>::iterator SendBufferIterator = SendBuffers.begin();

    for(; SendBufferIterator != SendBuffers.end(); ++ SendBufferIndex, ++ SendBufferIterator)
    {
        char * CurrentSendBuffer = *SendBufferIterator;

        if(!FirstFile || FirstFile->Position != SendBufferIndex)
        {
            Socket.Send(CurrentSendBuffer,
                CurrentSendBuffer == SendBuffers.back() ? LastSendBufferSize : WebserverInternal::SendBufferSize);

            Server.ReturnSendBuffer(CurrentSendBuffer);

            continue;
        }

        int StartAt = 0;

        for(;;)
        {
            Socket.Send(CurrentSendBuffer + StartAt, FirstFile->Offset - StartAt);
            StartAt += FirstFile->Offset - StartAt;

            FirstFile->Send(Socket);

            WebserverClient::Outgoing::File * Next = FirstFile->Next;
            delete FirstFile;
            FirstFile = Next;

            if(FirstFile && FirstFile->Position == SendBufferIndex)
                continue;

            if(CurrentSendBuffer == SendBuffers.back())
            {
                Socket.Send(CurrentSendBuffer + StartAt, LastSendBufferSize - StartAt);
                Server.ReturnSendBuffer(CurrentSendBuffer);
            }
            else
            {
                Socket.Send(CurrentSendBuffer + StartAt, WebserverInternal::SendBufferSize - StartAt);
                Server.ReturnSendBuffer(CurrentSendBuffer);
            }

            break;
        }
    }

    Socket.Flush();


    /* Close the connection if this is HTTP/1.0 without a Connection: Keep-Alive header */

    if((!stricmp(Client.Input.Version, "HTTP/1.0")) && stricmp(Client.Request.Header("Connection"), "Keep-Alive"))
        Client.Socket.Disconnect();


    /* All the send buffers have been returned now, so the send buffers in SendBuffers aren't ours */

    SendBuffers.clear();


    /* Both input and output are reset by Respond() */

    Client.Input.Reset();
    Client.Output.Reset();
}


/* Called upon initialisation, and at the end of each response to prepare for a new one */

void WebserverClient::Outgoing::Reset()
{
    TotalFileSize = LastSendBufferSize = 0;

    strcpy(ResponseCode,  "200 OK"    );
    strcpy(Charset,       "UTF-8"     );
    strcpy(MimeType,      "text/html" );

    Headers.Clear();
    Cookies.Clear();

}


void WebserverClient::Outgoing::AddSend(const char * Data, size_t Size)
{
    if(Size == -1)
        Size = strlen(Data);

    if(!Size)
        return;

    if(!SendBuffers.size())
    {
        LastSendBufferSize = 0;
        SendBuffers.push_back(Server.BorrowSendBuffer());
    }

    char * LastSendBuffer = SendBuffers.back();

    if(Size < (WebserverInternal::SendBufferSize - LastSendBufferSize))
    {
        /* There's enough space in the last send buffer for this data */

        memcpy(LastSendBuffer + LastSendBufferSize, Data, Size);
        LastSendBufferSize += Size;

        return;
    }
    
    /* Not enough room in the last send buffer.  Fill it up with us much as we can. */

    int Maximum = (WebserverInternal::SendBufferSize - LastSendBufferSize);

    memcpy(LastSendBuffer + LastSendBufferSize, Data, Maximum);

    LastSendBufferSize += Maximum;
    Size -= Maximum;

    /* Now create a new send buffer and recurse */

    LastSendBufferSize = 0;
    SendBuffers.push_back(Server.BorrowSendBuffer());

    AddSend(Data, Size);
}

void WebserverClient::Outgoing::AddFileSend(const char * Filename, int FilenameLength, lw_i64 Size)
{
    TotalFileSize += Size;

    WebserverClient::Outgoing::File * File = FirstFile;

    if(!File)
    {
        File = FirstFile = new WebserverClient::Outgoing::File;
    }
    else
    {
        while(File->Next)
            File = File->Next;

        File->Next = new WebserverClient::Outgoing::File;
        File = File->Next;
    }

    File->Next = 0;
    File->Position = SendBuffers.size() - 1;
    File->Offset = LastSendBufferSize;

    memcpy(File->Filename, Filename, FilenameLength);
}

void WebserverClient::Outgoing::File::Send(Lacewing::Server::Client &Socket)
{
    if(*(unsigned char *) Filename == 0xFF)
    {
        Socket.Send(*(const char **)(Filename + 1),
                        *(unsigned int *) (Filename + 1 + sizeof(const char *)));

        return;
    }

    Socket.SendFile(Filename);
}


