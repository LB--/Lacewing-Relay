
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

#ifdef __linux__
    #define LacewingLinuxSendfile
#else
    #ifdef __APPLE__
        #define LacewingMacSendFile
    #else
        #ifdef __FreeBSD__
            #define LacewingFreeBSDSendFile
        #else
            #error "Don't know how to use sendfile() on this platform"
        #endif
    #endif
#endif


/* This is just a thin wrapper around the various sendfile() calls offered by different *nix-es.

   Sends a file from SourceFD to DestFD.  Returns true for success, false for failure.
   On success, the number of bytes sent will be added to Offset and subtracted from Size. */

bool LacewingSendFile(int SourceFD, int DestFD, off_t &Offset, off_t &Size)
{
    #ifdef LacewingLinuxSendfile
        
        int Sent = sendfile(DestFD, SourceFD, &Offset, Size);

        if(Sent == -1)
        {
            if(errno == EAGAIN)
                return true;

            return false;
        }

        /* Linux automatically adjusts the offset */

        Size -= Sent;        
        return true;

    #endif

    #ifdef LacewingFreeBSDSendFile

        off_t Sent;

        if(sendfile(SourceFD, DestFD, Offset, Size, 0, &Sent, 0) != 0)
        {
            /* EAGAIN might still have sent some bytes on BSD */

            if(errno != EAGAIN)
                return false;
        }
                                    
        Offset += Sent;
        Size   -= Sent;
    
        return true;

    #endif

    #ifdef LacewingMacSendFile

        off_t Sent = Size;

        if(sendfile(SourceFD, DestFD, Offset, &Sent, 0, 0) != 0)
        {
            /* EAGAIN might still have sent some bytes on OS X */

            if(errno != EAGAIN)
                return false;
        }
                  
        Offset += Sent;
        Size   -= Sent;
    
    #endif
}


/* And these are the high level file transfer classes used by Lacewing::Server */

class FileTransfer
{
public:

    /* These will return false when the file transfer is complete (or failed) */

    virtual bool WriteReady () = 0;
    virtual bool ReadReady () = 0;
};

class RawFileTransfer : public FileTransfer
{
protected:

    int FromFD;
    int ToFD;

    off_t Offset, Size;

public:

    RawFileTransfer (int FromFD, int ToFD, off_t Offset, off_t Size)
    {
        this->FromFD = FromFD;
        this->ToFD   = ToFD;
        this->Offset = Offset;
        this->Size   = Size;
    }

    ~ RawFileTransfer ()
    {
        close(FromFD);
    }

    bool WriteReady()
    {
        if(!LacewingSendFile(FromFD, ToFD, Offset, Size))
            return false;

        if(!Size)
            return false;

        return true;
    }

    bool ReadReady()
    {
        return true;
    }
};

/* With SSL, we can't use sendfile() at all, so Lacewing reads chunks of the file
   into userland with read() and sends them with SSL_write() */

class SSLFileTransfer : public FileTransfer
{
protected:

    char Chunk[1024 * 256];
    int ChunkSize;
    
    /* Returns true if there's a chunk loaded that needs sending, or
       false if there was an error (assume EOF) */

    bool Read()
    {
        if(ChunkSize != -1)
        {
            /* There's already a chunk to send */
            return true;
        }
        
        int BytesLeft = Bytes - BytesRead;
        int ToRead = BytesLeft > sizeof(Chunk) ? sizeof(Chunk) : BytesLeft;

        if(!ToRead)
            return false;
        
        if((ChunkSize = read(FromFD, Chunk, ToRead)) == -1)
            return false;

        BytesRead += ChunkSize;
        return true;
    }

    int FromFD;
    SSL * To;

    off_t Bytes, BytesRead;

    enum
    {
        SendOnReadReady,
        SendOnWriteReady

    } SendOn;

    bool Ready()
    {
        if(!Read())
            return false;

        int Bytes = SSL_write(To, Chunk, ChunkSize);

        if(Bytes == 0)
            return false;

        if(Bytes > 0)
        {
            ChunkSize = -1;

            if(!Read())
                return false;

            SendOn = SendOnWriteReady;
            return true;
        }

        switch(SSL_get_error(To, Bytes))
        {
            case SSL_ERROR_WANT_READ:

                SendOn = SendOnReadReady;
                break;

            case SSL_ERROR_WANT_WRITE:

                SendOn = SendOnWriteReady;
                break;

            default:
                return false;
        };

        return true;
    }

public:

    SSLFileTransfer (int FromFD, SSL * To, off_t Bytes)
    {
        this->FromFD    = FromFD;
        this->To        = To;
        this->Bytes     = Bytes;
        this->BytesRead = 0;

        SendOn = SendOnWriteReady;
        ChunkSize = -1;
    }

    ~ SSLFileTransfer ()
    {
        close(FromFD);
    }

    bool WriteReady()
    {
        if(SendOn != SendOnWriteReady)
            return true;

        return Ready();
    }

    bool ReadReady()
    {
        if(SendOn != SendOnReadReady)
            return true;

        return Ready();
    }
};





