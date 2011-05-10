
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
#include "Webserver.Map.h"

struct BodyProcBase
{
    enum { Error, Continue, Done } State;

    virtual size_t Process(char *, size_t) = 0;
    virtual void CallRequestHandler     () = 0;
};

struct Multipart;

struct UploadInternal
{
    Lacewing::Webserver::Upload Upload;

    Multipart * Parent;

    const char * FormElement;
    const char * Filename;
    
    const char * AutoSaveFilename;
    FILE * AutoSaveFile;

    Map Copier;

    inline UploadInternal()
    {
        Upload.InternalTag = this;
        Upload.Tag         = 0;

        AutoSaveFilename = "";
        AutoSaveFile     = 0;
    }

    inline ~UploadInternal()
    {
        DebugOut("Free upload!");

    }
};

class WebserverClient;

struct Multipart : public BodyProcBase
{
    WebserverClient &Client;

    char Boundary         [512];
    char FinalBoundary    [512];
    char CRLFThenBoundary [512];

    bool   InHeaders, InFile;
    char * Header;
    void   ProcessHeader();

    Multipart * Child, * Parent;

     Multipart(WebserverClient &, const char * ContentType);
    ~Multipart();

    size_t Process(char *, size_t);
    void   CallRequestHandler   ();

    void ProcessDispositionPair(char * Pair);
    void ToFile(const char *, size_t);

    Map Disposition;
    Map Headers;

    vector<Lacewing::Webserver::Upload *> Uploads;
    UploadInternal * CurrentUpload;

    MessageBuilder Buffer;
};

class WebserverInternal
{
public:

    ThreadTracker      Threads;
    EventPumpInternal  &EventPump;

    const static size_t SendBufferSize    = 1024 * 32;  /* Maximum of 32 KB wasted per SendFile/SendConstant */
    const static size_t SendBufferBacklog = 32;         /* 256 KB allocated initially and when the server runs out */

    Lacewing::Server * Socket, * SecureSocket;

    Backlog<Lacewing::Server::Client, WebserverClient>
        ClientBacklog;

    Lacewing::Sync Sync_SendBuffers;
    list<char *> SendBuffers;

    char * BorrowSendBuffer();
    void ReturnSendBuffer(char * SendBuffer);

    Lacewing::SpinSync Sync_Sessions;
    map<string, Map> Sessions;

    MD5Hasher MD5;

    bool AutoFinish;

    static void SocketConnect(Lacewing::Server &, Lacewing::Server::Client &);
    static void SocketDisconnect(Lacewing::Server &, Lacewing::Server::Client &);
    static void SocketReceive(Lacewing::Server &, Lacewing::Server::Client &, char * Buffer, int Size);
    static void SocketError(Lacewing::Server &, Lacewing::Error &);

    inline void PrepareSocket()
    {
        if(!Socket)
        {
            Socket = new Lacewing::Server(EventPump.EventPump);

            Socket->Tag = this;

            Socket->onConnect    (SocketConnect);
            Socket->onDisconnect (SocketDisconnect);
            Socket->onReceive    (SocketReceive);
            Socket->onError      (SocketError);
        }
    }

    inline void PrepareSecureSocket()
    {
        if(!SecureSocket)
        {
            SecureSocket = new Lacewing::Server(EventPump.EventPump);
    
            SecureSocket->Tag = this;

            SecureSocket->onConnect    (SocketConnect);
            SecureSocket->onDisconnect (SocketDisconnect);
            SecureSocket->onReceive    (SocketReceive);
            SecureSocket->onError      (SocketError);
        }
    }

    Lacewing::Webserver &Webserver;

    Lacewing::Webserver::HandlerError        HandlerError;
    Lacewing::Webserver::HandlerGet          HandlerGet;
    Lacewing::Webserver::HandlerPost         HandlerPost;
    Lacewing::Webserver::HandlerHead         HandlerHead;
    Lacewing::Webserver::HandlerConnect      HandlerConnect;
    Lacewing::Webserver::HandlerDisconnect   HandlerDisconnect;
    Lacewing::Webserver::HandlerUploadStart  HandlerUploadStart;
    Lacewing::Webserver::HandlerUploadChunk  HandlerUploadChunk;
    Lacewing::Webserver::HandlerUploadDone   HandlerUploadDone;
    Lacewing::Webserver::HandlerUploadPost   HandlerUploadPost;

    inline WebserverInternal(Lacewing::Webserver &_Webserver, EventPumpInternal &_EventPump)
            : Webserver(_Webserver), EventPump(_EventPump)
    {
        Socket = SecureSocket = 0;

        HandlerError        = 0;
        HandlerGet          = 0;
        HandlerPost         = 0;
        HandlerHead         = 0;
        HandlerConnect      = 0;
        HandlerDisconnect   = 0;
        HandlerUploadStart  = 0;
        HandlerUploadChunk  = 0;
        HandlerUploadDone   = 0;
        HandlerUploadPost   = 0;

        AutoFinish = true;
    }
};

class WebserverClient
{
public:

    Lacewing::Webserver::Request Request;    
    Lacewing::Server::Client &Socket;

    struct Outgoing;

    class Incoming
    {
    protected:

        MessageBuilder Buffer;

        lw_i64 BodyRemaining;

        /* ProcessLine will call either ProcessFirstLine or ProcessHeader, and update State */

        void ProcessLine      (char * Line);

        void ProcessFirstLine (char * Line);
        void ProcessHeader    (char * Line);

        void ProcessCookie    (char * Cookie);

    public:

        int State;

        char Method     [8];
        char Version    [16];
        char URL        [4096];
        char Hostname   [128];

        Map Headers, GetItems, PostItems, Cookies;

        WebserverClient &Client;
        Incoming(WebserverClient &);

        void Process(char * Buffer, int Size);
        void Reset();

        BodyProcBase * BodyProcessor;

        friend struct WebserverClient::Outgoing;

    } Input;

    struct Outgoing
    {
        struct File
        {
            char Filename[MAX_PATH];
            int  Position, Offset;

            File * Next;

            void Send(Lacewing::Server::Client &Socket);
        };

        WebserverInternal   &Server;
        WebserverClient     &Client;

        /* For before the response body */

        char ResponseCode[64];
        char Charset[8];
        char MimeType[64];

        Map Headers, Cookies;


        /* For the response body */

        list<char *> SendBuffers;
        size_t LastSendBufferSize;

        File * FirstFile;
        lw_i64 TotalFileSize;


        Outgoing(WebserverInternal &, WebserverClient &);

        void RunHandler ();
        void Respond    ();

        void AddFileSend(const char * Filename, int FilenameLength, lw_i64 Size);
        void AddSend(const char * Buffer, size_t Size);
        void Reset();

    } Output;

    WebserverInternal &Server;

    WebserverClient(Lacewing::Server::Client &_Socket);
    
    bool ConnectHandlerCalled, Disconnected, RequestUnfinished;
    Map  Cookies, SessionDataCopies;
    
    bool Secure;
};

