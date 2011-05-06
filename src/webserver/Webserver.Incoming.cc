
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

    /* States 0 and 1 read lines */

    if(State < 2)
    {
        for(int i = 0; i < Size; ++ i)
        {
            if(*(short *) (Buffer + i) == *(const short *) "\r\n")
            {
                Buffer[i ++] = 0;

                if(this->Buffer.Size)
                {
                    this->Buffer.Add(Buffer, i);
                    ProcessLine(this->Buffer.Buffer);

                    this->Buffer.Reset();
                }
                else
                    ProcessLine(Buffer);

                int SizeLeft = Size - i - 1;

                if(SizeLeft > 0)
                    Process(Buffer + (Size - SizeLeft), SizeLeft);

                return;
            }

            continue;
        }

        this->Buffer.Add(Buffer, Size);

        return;
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

            if(ExtraBytes > 0)
                Process(Buffer + (Size - ExtraBytes), ExtraBytes);

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

                char * NameDecoded = (char *) malloc(strlen(Name) + 12);
                char * ValueDecoded = (char *) malloc(strlen(Value) + 12);

                if(!URLDecode(Name, NameDecoded, strlen(Name) + 8) || !URLDecode(Value, ValueDecoded, strlen(Value) + 8))
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
    switch(State)
    {
    case 0:

        /* First line */

        ProcessFirstLine(Line);
        
        State = 1;
        break;

    case 1:

        /* Header */

        if(*Line)
        {
            ProcessHeader(Line);
        }
        else
        {
            /* Blank line marks end of headers */

            if(!strcmp(Method, "GET") || !strcmp(Method, "HEAD") ||
                    (BodyRemaining = _atoi64(Client.Request.Header("Content-Length"))) == 0)
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

        break;
    };

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

                char * NameDecoded = (char *) malloc(strlen(Name) + 12);
                char * ValueDecoded = (char *) malloc(strlen(Value) + 12);

                if(!URLDecode(Name, NameDecoded, strlen(Name) + 8) || !URLDecode(Value, ValueDecoded, strlen(Value) + 8))
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
            if(i[0] == '.' && i[1] == '.' && (i[2] == '/' || i[2] == '\\'))
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


        /* Default - store the header */

        Headers.Set(Name, Line);

    } while(0);
}


/* Called upon initialisation, and at the end of each request to prepare for a new one */

void WebserverClient::Incoming::Reset()
{
    State = 0;

    Headers   .Clear();
    GetItems  .Clear();
    PostItems .Clear();
    Cookies   .Clear();

    BodyRemaining = 0;

    delete BodyProcessor;
    BodyProcessor = 0;

}

