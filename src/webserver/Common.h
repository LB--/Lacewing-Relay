
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

#include "../Common.h"
#include "../../deps/http-parser/http_parser.h"

#include "Map.h"

class HTTPClient;

struct Webserver::Upload::Internal
{
    Lacewing::Webserver::Upload Upload;

    const char * FormElement;
    const char * Filename;
    
    const char * AutoSaveFilename;
    FILE * AutoSaveFile;

    Map Headers, Copier;

    inline Internal ()
    {
        Upload.internal = this;
        Upload.Tag         = 0;

        AutoSaveFilename = "";
        AutoSaveFile     = 0;
    }

    virtual inline ~ Internal ()
    {
        DebugOut("Free upload!");
    }

    virtual const char * Header (const char * Name) = 0;
};

struct Webserver::Internal
{
    Lacewing::Pump &Pump;

    Lacewing::Server * Socket, * SecureSocket;
    Lacewing::Timer Timer;

    struct Session
    {
        lw_i64 ID_Part1;
        lw_i64 ID_Part2;

        Session * Next;

        Map Data;

    } * FirstSession;

    Session * FindSession (const char * SessionID_Hex);

    bool AutoFinish;

    static void SocketConnect(Lacewing::Server &, Lacewing::Server::Client &);
    static void SocketDisconnect(Lacewing::Server &, Lacewing::Server::Client &);
    static void SocketReceive(Lacewing::Server &, Lacewing::Server::Client &, char * Buffer, size_t);
    static void SocketError(Lacewing::Server &, Lacewing::Error &);

    inline void PrepareSocket()
    {
        if(!Socket)
        {
            Socket = new Lacewing::Server (Pump);

            Socket->Tag = this;

            Socket->onConnect    (SocketConnect);
            Socket->onDisconnect (SocketDisconnect);
            Socket->onReceive    (SocketReceive);
            Socket->onError      (SocketError);
        }

        StartTimer ();
    }

    inline void PrepareSecureSocket()
    {
        if(!SecureSocket)
        {
            SecureSocket = new Lacewing::Server (Pump);
    
            SecureSocket->Tag = this;

            SecureSocket->onConnect    (SocketConnect);
            SecureSocket->onDisconnect (SocketDisconnect);
            SecureSocket->onReceive    (SocketReceive);
            SecureSocket->onError      (SocketError);
        }
        
        StartTimer ();
    }

    int Timeout;

    inline void StartTimer ()
    {
        #ifdef LacewingTimeoutExperiment

            if (Timer.Started())
                return;

            Timer.Start (Timeout * 1000);
        
        #endif
    }

    inline void StopTimer ()
    {
        Timer.Stop ();
    }

    Lacewing::Webserver &Webserver;

    struct
    {
        HandlerError        Error;
        HandlerGet          Get;
        HandlerPost         Post;
        HandlerHead         Head;
        HandlerUploadStart  UploadStart;
        HandlerUploadChunk  UploadChunk;
        HandlerUploadDone   UploadDone;
        HandlerUploadPost   UploadPost;
        HandlerDisconnect   Disconnect;

    } Handlers;

    inline Internal (Lacewing::Webserver &_Webserver, Lacewing::Pump &_Pump)
            : Webserver (_Webserver), Pump (_Pump), Timer (_Pump)
    {
        Socket = SecureSocket = 0;

        memset (&Handlers, 0, sizeof (Handlers));

        AutoFinish = true;
        FirstSession = 0;

        Timeout = 5;

        Timer.Tag = this;
        Timer.onTick (TimerTickStatic);
    }

    static void TimerTickStatic (Lacewing::Timer &);
    void TimerTick ();
};

class WebserverClient;

struct Webserver::Request::Internal
{
    Lacewing::Webserver::Request Public; 

    Webserver::Internal &Server;
    WebserverClient &Client;

    Internal (Webserver::Internal &_Server, WebserverClient &_Client);
    ~ Internal ();

    void Clean ();
    

    /* Input */

    char Method     [16];
    char URL        [4096];
    char Hostname   [128];

    Map InHeaders, InCookies, GetItems, PostItems;
    
    void In_Method (const char * Method);
    void In_Header (const char * Name, char * Value);
    bool In_URL (char * URL);
    
    /* The protocol implementation can use this for any intermediate
     * buffering, providing it contains the request body (if any) when
     * the handler is called.
     */

    HeapBuffer Buffer;

    void ParsePostData ();
    bool ParsedPostData;


    /* Output */

    char Status [64];

    Map OutHeaders, OutCookies;
    
    void BeforeHandler ();
    void AfterHandler ();

    void RunStandardHandler ();    

    bool Responded;
    void Respond ();
};

class WebserverClient
{
public:

    bool Secure;

    Lacewing::Server::Client &Socket;
    Webserver::Internal &Server;

    int Timeout;

    WebserverClient (Webserver::Internal &, Lacewing::Server::Client &, bool Secure);
    virtual ~ WebserverClient ();

    virtual void Tick () = 0;

    virtual void Process (char * Buffer, size_t Size) = 0;
    virtual void Respond (Webserver::Request::Internal &Request) = 0;
    virtual void Dead () = 0;
    
    virtual bool IsSPDY () = 0;
};

#include "http/HTTP.h"


