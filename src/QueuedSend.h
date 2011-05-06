
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

#ifndef LacewingQueuedSend
#define LacewingQueuedSend

namespace QueuedSendType
{
    enum Type
    {
        /* Will be written when the last queue item has finished being written. */
          
          Data,
          File,

        /* Writing will be attempted again after the next SSL_read has been performed.
           Always gets pushed to the front of the queue.  Only one can exist in the queue. */
           
          SSLWriteWhenReadable
    };
}

struct QueuedSend
{
    QueuedSendType::Type Type;

    MessageBuilder Data;
    int DataOffset;

    char * Filename;
    
    lw_i64 FileSize;
    lw_i64 FileOffset;

    QueuedSend(QueuedSendType::Type Type)
    {
        this->Type = Type;

        Filename   = 0;
        Next       = 0;
        DataOffset = 0;
    }

    ~QueuedSend()
    {
        free(Filename);
    }

    QueuedSend * Next;
};

struct QueuedSendManager
{
    Lacewing::Sync Sync;

    QueuedSend * First, * Last;
    
    inline QueuedSendManager()
    {
        First = Last = 0;
    }
    

    inline void Add(QueuedSend * Data)
    {
        Lacewing::Sync::Lock Lock(Sync);

        if(!First)
        {
            Last = First = Data;
            return;
        }
        
        Last->Next = Data;
        Last = Data;
    }

    inline void Add(const char * Buffer, int Size)
    {
        Lacewing::Sync::Lock Lock(Sync);

        if(Last && (Last->Type == QueuedSendType::Data || Last->Type == QueuedSendType::SSLWriteWhenReadable))
        {
            Last->Data.Add(Buffer, Size);
            return;
        }

        QueuedSend * New = new QueuedSend(QueuedSendType::Data);
        New->Data.Add(Buffer, Size);

        Add(New);
    }

    inline void Add(const char * Filename, lw_i64 Offset, lw_i64 Size)
    {
        Lacewing::Sync::Lock Lock(Sync);

        QueuedSend * New = new QueuedSend(QueuedSendType::File); 

        New->Filename   = strdup(Filename);
        New->FileOffset = Offset;
        New->FileSize   = Size;

        Add(New);
    }


    inline void AddFront(QueuedSend * Data)
    {
        Lacewing::Sync::Lock Lock(Sync);

        if(!First)
        {
            Last = First = Data;
            return;
        }
        
        Data->Next = First;
        First = Data;
    }

    inline void AddSSLWriteWhenReadable(const char * Buffer, int Size)
    {
        Lacewing::Sync::Lock Lock(Sync);

        QueuedSend * New = new QueuedSend(QueuedSendType::SSLWriteWhenReadable);
        New->Data.Add(Buffer, Size);

        AddFront(New);
    }
};

#endif

