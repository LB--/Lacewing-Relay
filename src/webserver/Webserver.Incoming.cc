
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

WebserverClient::Incoming::Incoming(WebserverClient &_Client) : Client(_Client)
{
    BodyProcessor = 0;

    Reset();
}

void WebserverClient::Incoming::Process(char * Buffer, int Size)
{
    if(Client.RequestUnfinished)
    {
        /* The application hasn't yet called Finish() for the last request, so this data
           can't be processed.  Buffer it to process when Finish() is called. */

        this->Buffer.Add(Buffer, Size);
        return;
    }

    /* State 0 : Line content
       State 1 : Got CR, need LF
       State 2 : Message body */

    if(State < 2)
    {
        for(char * i = Buffer; *i; )
        {
            if(State == 0 && *i == '\r')
            {
                State = 1;
            }
            else if(*i == '\n')
            {
                if(this->Buffer.Size)
                {
                    this->Buffer.Add (Buffer, -1);
                    this->Buffer.Add <char> (0);

                    ProcessLine(this->Buffer.Buffer);

                    this->Buffer.Reset();
                }
                else
                {
                    i [State == 1 ? -1 : 0] = 0;
                    ProcessLine(Buffer);
                }

                Size -= (++ i) - Buffer;
                Buffer = i;

                if(State == -1 || State >= 2)
                    break;

                State = 0;
                continue;
            }
            else if(State == 1)
            {
                /* The only thing valid after \r is \n */

                State = -1;
                break;
            }
                
            ++ i;
        }
            
        if(State == -1)
        {
            Client.Socket.Disconnect();
            return;
        }

        if(State < 2)
        {
            this->Buffer.Add(Buffer, Size);
            return;
        }
    }

    /* State >= 2 is the request body */

    if(BodyProcessor)
    {
        /* TODO : Is the ExtraBytes stuff even necessary?  Post requests
           can't be pipelined. */

        int ExtraBytes = BodyProcessor->Process(Buffer, Size);
     
        if(BodyProcessor->State == BodyProcBase::Done)
        {       
            Client.RequestUnfinished = true;
            Client.Output.RunHandler();

          /*  if(ExtraBytes > 0)
                Process(Buffer + (Size - ExtraBytes), ExtraBytes); */

            return;
        }

        if(BodyProcessor->State == BodyProcBase::Error)
        {       
            DebugOut("BodyProcessor reported error - killing client");
            Client.Request.Disconnect();

            return;
        }

        return;
    }

    /* No body processor = standard form post data */

    int ToRead = BodyRemaining < Size ? BodyRemaining : Size;
    this->Buffer.Add(Buffer, ToRead);

    BodyRemaining -= ToRead;
    Size -= ToRead;
    
    if(BodyRemaining == 0)
    {
        Lacewing::Webserver::Request &Request = Client.Request;

        this->Buffer.Add<char>(0);
        char * PostData = this->Buffer.Buffer;

        /* Parse the POST data into name/value pairs.  Using BeginsWith to check the Content-Type to
           exclude the charset if specified (eg. application/x-www-form-urlencoded; charset=UTF-8) */

        if(*PostData)
        {
            for(;;)
            {
                char * Name = PostData;
                char * Value = strchr(Name, '=');

                if(!Value)
                    break;

                *(Value ++) = 0;

                char * Next = strchr(Value, '&');
                
                if(Next)
                    *(Next ++) = 0;

                char * NameDecoded = (char *) malloc(strlen(Name) + 1);
                char * ValueDecoded = (char *) malloc(strlen(Value) + 1);

                if(!URLDecode(Name, NameDecoded, strlen(Name) + 1) || !URLDecode(Value, ValueDecoded, strlen(Value) + 1))
                {
                    free(NameDecoded);
                    free(ValueDecoded);
                }
                else
                    PostItems.SetAndGC(NameDecoded, ValueDecoded);

                if(!Next)
                    break;

                PostData = Next;
            }
        }

        this->Buffer.Reset();

        Client.RequestUnfinished = true;
        Client.Output.RunHandler();

        /* Now we wait for Finish() to call Respond().  It may have done so already, or the application
           may call Finish() later. */
    }

    /* If Size is > 0 here, the client must be pipelining requests. */

    if(Size > 0)
        Process(Buffer, Size);
}

void WebserverClient::Incoming::ProcessLine(char * Line)
{
    do
    {   if(!*Method)
        {
            ProcessFirstLine(Line);
            break;
        }

        if(*Line)
        {
            ProcessHeader(Line);
            break;
        }

        /* Blank line marks end of headers */

        if(!strcmp(Method, "GET") || !strcmp(Method, "HEAD") ||
                (BodyRemaining = _atoi64(Client.Request.Header("Content-Length"))) <= 0)
        {
            /* No body - this is a complete request done */
        
            Client.RequestUnfinished = true;
            Client.Output.RunHandler();

            break;
        }


        /* The request has a body.  BodyRemaining has been set, and definitely isn't 0. */

        const char * ContentType = Client.Request.Header("Content-Type");

        if(BeginsWith(ContentType, "application/x-www-form-urlencoded"))
        {
            /* Support for standard form data (application/x-www-form-urlencoded) is tangled into the webserver code */

            State = 2;
            break;
        }


        /* Other types of body get handled by the body processors (currently only multipart) */

        if(BeginsWith(ContentType, "multipart"))
        {
            BodyProcessor = new Multipart(Client, ContentType);
            
            State = 3;
            break;
        }

        /* Unknown content type */

        State = -1;
    }
    while(0);

    if(State == -1)
    {
        /* Parsing error */
        Client.Socket.Disconnect();
    }
}

void WebserverClient::Incoming::ProcessFirstLine(char * Line)
{
    /* Method (eg GET, POST, HEAD) */

    {   char * Method = Line;

        if(!(Line = strchr(Line, ' ')))
        {
            State = -1;
            return;
        }

        *(Line ++) = 0;

        if(strlen(Method) > (sizeof(this->Method) - 1))
        {
            State = -1;
            return;
        }

        strcpy(this->Method, Method);
    }

    /* URL */

    {   char * URL = Line;

        if(!(Line = strchr(Line, ' ')))
        {
            State = -1;
            return;
        }

        *(Line ++) = 0;

        if(*URL == '/')
            ++ URL;

        /* Strip the GET data from the URL and add it to GetItems, decoded */

        char * GetData = strchr(URL, '?');

        if(GetData)
        {
            *(GetData ++) = 0;

            for(;;)
            {
                char * Name = GetData;
                char * Value = strchr(Name, '=');

                if(!Value)
                    break;

                *(Value ++) = 0;

                char * Next = strchr(Value, '&');
                
                if(Next)
                    *(Next ++) = 0;

                char * NameDecoded = (char *) malloc(strlen(Name) + 1);
                char * ValueDecoded = (char *) malloc(strlen(Value) + 1);

                if(!URLDecode(Name, NameDecoded, strlen(Name) + 1) || !URLDecode(Value, ValueDecoded, strlen(Value) + 1))
                {
                    free(NameDecoded);
                    free(ValueDecoded);
                }
                else
                    GetItems.SetAndGC(NameDecoded, ValueDecoded);

                if(!Next)
                    break;

                GetData = Next;
            }
        }

        /* Make an URL decoded copy of the URL with the GET data stripped */
        
        if(!URLDecode(URL, this->URL, sizeof(this->URL)))
        {
            State = -1;
            return;
        }

        /* Check for directory traversal in the URL, and disconnect the client if it's found.
           Also replace any backward slashes with forward slashes.  */

        for(char * i = this->URL; *i; ++ i)
        {
            if(i[0] == '.' && i[1] == '.')
            {
                State = -1;
                return;
            }

            if(*i == '\\')
                *i = '/';
        }
    }

    /* HTTP version */

    if(stricmp(Line, "HTTP/1.1") && stricmp(Line, "HTTP/1.0"))
    {
        State = -1;
        return;
    }

    strcpy(this->Version, Line);
}

void WebserverClient::Incoming::ProcessCookie(char * Cookie)
{
    for(;;)
    {
        char * Name = Cookie;

        if(!(Cookie = strchr(Cookie, '=')))
        {
            State = -1;
            return;
        }

        *(Cookie ++) = 0;

        char * Next = strchr(Cookie, ';');

        if(Next)
            *(Next ++) = 0;

        while(*Name == ' ')
            ++ Name;

        while(*Name && Name[strlen(Name - 1)] == ' ')
            Name[strlen(Name - 1)] = 0;

        /* The copy in Input doesn't get modified, so the response generator
           can determine which cookies have been changed. */

        Client.Cookies.Set      (Name, Cookie);
        Client.Input.Cookies.Set(Name, Cookie);
        
        if(!(Cookie = Next))
            break;
    }
}

void WebserverClient::Incoming::ProcessHeader(char * Line)
{
    char * Name = Line;

    if(!(Line = strchr(Line, ':')))
    {
        State = -1;
        return;
    }

    *(Line ++) = 0;

    while(*Line == ' ')
        ++ Line;

    do
    {
        if(!stricmp(Name, "Cookie"))
        {
            /* Cookies are stored separately */

            ProcessCookie(Line);
            break;
        }

        if(!stricmp(Name, "Host"))
        {
            /* The hostname gets stored separately with the port removed for
               the Request.Hostname() function (the raw header is still saved) */

            strncpy(Hostname, Line, sizeof(Hostname));

            for(char * i = Hostname; *i; ++ i)
            {
                if(*i == ':')
                {
                    *i = 0;
                    break;
                }
            }
        }   

        /* Store the header */

        Headers.Set(Name, Line);

    } while(0);
}


/* Called upon initialisation, and at the end of each request to prepare for a new one */

void WebserverClient::Incoming::Reset()
{
    State = 0;
    *Method = 0;

    Headers   .Clear();
    GetItems  .Clear();
    PostItems .Clear();
    Cookies   .Clear();

    BodyRemaining = 0;

    delete BodyProcessor;
    BodyProcessor = 0;

}

