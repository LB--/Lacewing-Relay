
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

/* TODO : rewrite this */

#include "../Common.h"

HTTPClient::MultipartProcessor::MultipartProcessor
    (HTTPClient &_Client, const char * ContentType) : Client(_Client)
{
    State = Continue;

    const char * Boundary = strstr(ContentType, "boundary=");

    if(!Boundary)
    {
        State = Error;
        return;
    }

    Boundary += 9;

    {   char * End = (char *) strchr(Boundary, ';');
        
        if(End)
            *End = 0;
    }

    if(strlen(Boundary) > (sizeof(this->Boundary) - 8))
    {
        State = Error;
        return;
    }

    sprintf(this->Boundary,           "--%s",     Boundary);
    sprintf(this->CRLFThenBoundary,   "\r\n--%s", Boundary);
    sprintf(this->FinalBoundary,      "%s--",     this->Boundary);

    InHeaders = InFile = false;
    Child     = Parent = 0;

    CurrentUpload = 0;
}

HTTPClient::MultipartProcessor::~MultipartProcessor()
{
    for (int i = 0; i < Uploads.Size; ++ i)
        delete Uploads [i]->internal;
    
    Uploads.Clear ();
}

size_t HTTPClient::MultipartProcessor::Process(char * Data, size_t Size)
{
    if(InFile && Child)
    {
        int SizeLeft = Child->Process(Data, Size);

        Data += Size - SizeLeft;
        Size = SizeLeft;

        if(Child->State == Done)
        {
            delete Child;
            Child = 0;
        }
    }

    size_t InFileOffset = InFile ? 0 : -1;

    for(size_t i = 0; i < Size; ++ i)
    {
        if(!InFile)
        {
            if(!InHeaders)
            {
                /* Before headers */

                if(lwp_begins_with(Data + i, this->Boundary))
                {
                    InHeaders = true;
                    i += strlen(this->Boundary);

                    while(i < Size && (Data[i] == '\r' || Data[i] == '\n'))
                        ++ i;

                    Header = Data + i --;
                }

                continue;
            }

            /* In headers */

            if(Data[i] == '\r' && Data[i + 1] == '\n')
            {
                Data[i] = 0;

                ProcessHeader();
                Header = Data + i + 2;

                if(InFile)
                    InFileOffset = i + 2;

                if(Child)
                    return Process(Data + i, Size - i);

                ++ i;
            }

            continue;
        }

        /* In file */

        if(lwp_begins_with(Data + i, this->CRLFThenBoundary))
        {
            ToFile(Data + InFileOffset, i - InFileOffset);

            if(CurrentUpload)
            {
                if(Parent)
                    Parent->Uploads.Push (&CurrentUpload->Upload);
                else
                    Uploads.Push (&CurrentUpload->Upload);

                if(!CurrentUpload->AutoSaveFile)
                {
                    /* Manual save */

                    if (Client.Server.Handlers.UploadDone)
                    {
                        Client.Server.Handlers.UploadDone
                            (Client.Server.Webserver, Client.Request, CurrentUpload->Upload);
                    }
                }
                else
                {
                    /* Auto save */

                    lwp_trace("Closing auto save file");

                    delete CurrentUpload->AutoSaveFile;
                    CurrentUpload->AutoSaveFile = 0;
                }

                CurrentUpload = 0;
            }
            else
            {
                HeapBuffer &buffer = Client.Request.Buffer;

                buffer.Add <char> (0);

                lw_nvhash_set (&Client.Request.PostItems,
                               lw_nvhash_get (&Disposition, "name", ""),
                               buffer.Buffer, lw_true);

                buffer.Reset();
            }

            lw_nvhash_clear (&Disposition);

            if(lwp_begins_with(Data + i + 2, this->FinalBoundary))
            {
                State = Done;
                return Size - i - (strlen(this->FinalBoundary) + 2);
            }

            InFile = false;
            -- i;

            continue;
        }
    }

    ToFile(Data + InFileOffset, Size - InFileOffset);

    return 0;
}

void HTTPClient::MultipartProcessor::ProcessHeader()
{
    if(!*Header)
    {
        /* Blank line marks end of headers */

        InHeaders = false;
        InFile    = true;

        /* If a filename is specified, the data is handled by the file upload
         * handlers, and is assigned an Upload structure.
         */
        
        const char * filename = lw_nvhash_get (&Disposition, "filename", 0);

        if (filename)
        {
            CurrentUpload = new HTTPUpload (Client.Request);
            
            CurrentUpload->Filename = strdup (filename);

            /* If this is a child multipart, the upload takes the form element
             * name from the parent disposition.
             */

            CurrentUpload->FormElement
                = strdup (lw_nvhash_get (Parent ?
                            &Parent->Disposition : &Disposition, "name", ""));
        
            if(Client.Server.Handlers.UploadStart)
            {
                Client.Server.Handlers.UploadStart
                    (Client.Server.Webserver, Client.Request, CurrentUpload->Upload);
            }

            return;
        }

        /* If a filename is not specified, the data is retrieved like a normal
         * form post item via POST().
         */
        
        return;
    }

    char * Name = Header;

    if(!(Header = strchr(Header, ':')))
    {
        State = Error;
        return;
    }

    *(Header ++) = 0;

    while(*Header == ' ')
        ++ Header;

    {  WebserverHeader header = { Name, Header };
       Headers.Push (header);
    }

    if(!strcasecmp(Name, "Content-Disposition"))
    {
        char * i;

        if((!(i = strchr(Header, ';'))) && (!(i = strchr(Header, '\r'))))
        {
            State = Error;
            return;
        }

        *(i ++) = 0;
        
        lw_nvhash_set (&Disposition, "Type", Header, lw_true);

        char * Begin = i;

        for(;;)
        {
            if ((i = strchr(Begin, ';')))
                *(i ++) = 0;
            else
                i = Begin + strlen(Begin) - 1;
            
            if(Begin >= i)
                break;

            while(*Begin == ' ')
                ++ Begin;

            ProcessDispositionPair(Begin);
            Begin = i + 1;
        }
    }

    if(!strcasecmp(Name, "Content-Type"))
    {
        if(lwp_begins_with(Header, "multipart"))
        {
            if(Parent)
            {
                /* A child processor can't have children */

                State = Error;
                return;
            }

            Child = new MultipartProcessor (Client, Header);
            Child->Parent = this;
        }
    }
}

void HTTPClient::MultipartProcessor::ProcessDispositionPair(char * Pair)
{
    char * Name = Pair;

    if(!(Pair = strchr(Pair, '=')))
    {
        State = Error;
        return;
    }

    *(Pair ++) = 0;

    while(*Pair == ' ')
        ++ Pair;

    if(*Pair == '"')
        ++ Pair;

    if(Pair[strlen(Pair) - 1] == '"')
        Pair[strlen(Pair) - 1] = 0;

    if((!strcasecmp(Name, "filename")) && strchr(Pair, '\\'))
    {
        /* Old versions of IE send the absolute path (!) */

        Pair += strlen(Pair) - 1;
        
        while(*Pair != '\\')
            -- Pair;

        ++ Pair;
    }
    
    lw_nvhash_set (&Disposition, Name, Pair, lw_true);
}

void HTTPClient::MultipartProcessor::ToFile(const char * Data, size_t Size)
{
    if(!Size)
        return;

    if(CurrentUpload)
    {
        if(!CurrentUpload->AutoSaveFile)
        {
            /* Manual save */

            if(Client.Server.Handlers.UploadChunk)
                Client.Server.Handlers.UploadChunk(Client.Server.Webserver,
                        Client.Request, CurrentUpload->Upload, Data, Size);

            return;
        }

        /* Auto save */

        CurrentUpload->AutoSaveFile->Write (Data, Size);
        return;
    }

    Client.Request.Buffer.Add (Data, Size);
}

void HTTPClient::MultipartProcessor::CallRequestHandler()
{
    Client.Request.BeforeHandler ();

    if(Client.Server.Handlers.UploadPost)
    {
        Client.Server.Handlers.UploadPost
           (Client.Server.Webserver, Client.Request, Uploads.Items, Uploads.Size);
    }

    Client.Request.AfterHandler ();
}

HTTPClient::HTTPUpload::HTTPUpload (Webserver::Request::Internal &request)
    : Webserver::Upload::Internal (request)
{
}

const char * HTTPClient::HTTPUpload::Header (const char * name)
{
    for (List <WebserverHeader>::Element * E = Headers.First; E; E = E->Next)
        if (!strcasecmp ((** E).Name, name))
            return (** E).Value;

    return "";
}

