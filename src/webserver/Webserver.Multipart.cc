
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

Multipart::Multipart(WebserverClient &_Client, const char * ContentType) : Client(_Client)
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

Multipart::~Multipart()
{
    for(vector<Lacewing::Webserver::Upload *>::iterator it = Uploads.begin(); it != Uploads.end(); ++ it)
        delete ((UploadInternal *) (*it)->InternalTag);
    
    Uploads.clear();
}

size_t Multipart::Process(char * Data, size_t Size)
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

    int InFileOffset = InFile ? 0 : -1;

    for(size_t i = 0; i < Size; ++ i)
    {
        if(!InFile)
        {
            if(!InHeaders)
            {
                /* Before headers */

                if(BeginsWith(Data + i, this->Boundary))
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

        if(BeginsWith(Data + i, this->CRLFThenBoundary))
        {
            ToFile(Data + InFileOffset, i - InFileOffset);

            if(CurrentUpload)
            {
                if(Parent)
                    Parent->Uploads.push_back(&CurrentUpload->Upload);
                else
                    Uploads.push_back(&CurrentUpload->Upload);

                if(!CurrentUpload->AutoSaveFile)
                {
                    /* Manual save */

                    if(Client.Server.HandlerUploadDone)
                        Client.Server.HandlerUploadDone(Client.Server.Webserver, Client.Request, CurrentUpload->Upload);
                }
                else
                {
                    /* Auto save */

                    DebugOut("Closing auto save file");

                    fclose(CurrentUpload->AutoSaveFile);
                    CurrentUpload->AutoSaveFile = 0;
                }

                CurrentUpload = 0;
            }
            else
            {
                Buffer.Add <char> (0);

                Client.Input.PostItems.Set(Disposition.Get("name"), Buffer.Buffer);
                Buffer.Reset();
            }

            Disposition.Clear();

            if(BeginsWith(Data + i + 2, this->FinalBoundary))
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

void Multipart::ProcessHeader()
{
    if(!*Header)
    {
        /* Blank line marks end of headers */

        InHeaders = false;
        InFile    = true;

        /* If a filename is specified, the data is handled by the PreFilePost/FilePostChunk/FilePostComplete
           handlers, and is assigned an Upload structure. */

        if(*Disposition.Get("filename"))
        {
            CurrentUpload = new UploadInternal;

            CurrentUpload->Parent   = this;
            CurrentUpload->Filename = CurrentUpload->Copier.Set("filename", Disposition.Get("filename"));
        

            /* If this is a child multipart, the upload takes the form element name from the parent disposition */

            CurrentUpload->FormElement = CurrentUpload->Copier.Set("name",
                                                    Parent ? Parent->Disposition.Get("name") : Disposition.Get("name"));
        
            if(Client.Server.HandlerUploadStart)
                Client.Server.HandlerUploadStart(Client.Server.Webserver, Client.Request, CurrentUpload->Upload);

            return;
        }

        /* If a filename is not specified, the data is retrieved like a normal form post item via POST(). */
        
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

    Headers.Set(Name, Header);

    if(!stricmp(Name, "Content-Disposition"))
    {
        char * i;

        if((!(i = strchr(Header, ';'))) && (!(i = strchr(Header, '\r'))))
        {
            State = Error;
            return;
        }

        *(i ++) = 0;
        
        Disposition.Set("Type", Header);

        char * Begin = i;

        for(;;)
        {
            if(i = strchr(Begin, ';'))
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

    if(!stricmp(Name, "Content-Type"))
    {
        if(BeginsWith(Header, "multipart"))
        {
            if(Parent)
            {
                /* A child multipart can't have children */

                State = Error;
                return;
            }

            Child = new Multipart(Client, Header);
            Child->Parent = this;
        }
    }
}

void Multipart::ProcessDispositionPair(char * Pair)
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

    if((!stricmp(Name, "filename")) && strchr(Pair, '\\'))
    {
        /* Old versions of IE send the absolute path (!) */

        Pair += strlen(Pair) - 1;
        
        while(*Pair != '\\')
            -- Pair;

        ++ Pair;
    }
    
    Disposition.Set(Name, Pair);
}

void Multipart::ToFile(const char * Data, size_t Size)
{
    LacewingAssert(Size >= 0);

    if(!Size)
        return;

    if(CurrentUpload)
    {
        if(!CurrentUpload->AutoSaveFile)
        {
            /* Manual save */

            if(Client.Server.HandlerUploadChunk)
                Client.Server.HandlerUploadChunk(Client.Server.Webserver, Client.Request, CurrentUpload->Upload, Data, Size);

            return;
        }

        /* Auto save */

        fwrite(Data, 1, Size, CurrentUpload->AutoSaveFile);
        return;
    }

    Buffer.Add(Data, Size);
}

void Multipart::CallRequestHandler()
{
    LacewingAssert(Client.RequestUnfinished);

    if(Client.Server.HandlerUploadPost)
        Client.Server.HandlerUploadPost(Client.Server.Webserver, Client.Request, &Uploads[0], Uploads.size());
}

const char * Lacewing::Webserver::Upload::Filename()
{
    return ((UploadInternal *) InternalTag)->Filename;
}

const char * Lacewing::Webserver::Upload::FormElementName()
{
    return ((UploadInternal *) InternalTag)->FormElement;
}

const char * Lacewing::Webserver::Upload::Header(const char * Name)
{
    return ((UploadInternal *) InternalTag)->Parent->Headers.Get(Name);
}

void Lacewing::Webserver::Upload::SetAutoSave()
{
    UploadInternal &Internal = *((UploadInternal *) InternalTag);

    if(*Internal.AutoSaveFilename)
        return;

    char Filename[MAX_PATH];
    NewTempFile(Filename, sizeof(Filename));

    Internal.AutoSaveFilename = Internal.Copier.Set("AutoSaveFilename", Filename);
    Internal.AutoSaveFile     = fopen(Filename, "wb");
}

const char * Lacewing::Webserver::Upload::GetAutoSaveFilename()
{
    return ((UploadInternal *) InternalTag)->AutoSaveFilename;
}

