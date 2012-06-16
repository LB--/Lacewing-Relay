
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

#ifndef LacewingHeapBuffer
#define LacewingHeapBuffer

namespace Lacewing
{
    /* TODO : Make this a Stream?  Or when can a vanilla Stream be used in
     * place of a HeapBuffer?  (thinking of HTTP response generation)
     */

    class HeapBuffer
    {
    public:
        
        size_t Size, Allocated, Offset;
        char * Buffer;

        inline HeapBuffer ()
        {
            this->Buffer = 0;
            Size = Allocated = Offset = 0;
        }

        inline ~ HeapBuffer ()
        {
            free (this->Buffer);
        }

        inline operator char * ()
        {
            return this->Buffer;
        }

        inline void Add (const char * buffer, size_t size)
        {
            if (size == -1)
                size = strlen (buffer);

            size_t new_size = this->Size + size;

            {   bool realloc = false;

                while (new_size > Allocated)
                {
                    Allocated = Allocated ? Allocated * 3 : 1024 * 4;
                    realloc = true;
                }

                if (realloc)
                {
                    this->Buffer = (char *)
                        ::realloc (this->Buffer, Allocated);
                }
            }

            memcpy (this->Buffer + this->Size, buffer, size);
            this->Size = new_size;
        }

        template <class T> inline void Add (T v)
        {
            Add ((char *) &v, sizeof (v));
        }

        inline HeapBuffer &operator << (char * s)                   
        {   Add (s, -1);                                      
            return *this;                                   
        }                                                   

        inline HeapBuffer &operator << (const char * s)             
        {   Add (s, -1);                                      
            return *this;                                   
        }                                                   

        inline HeapBuffer &operator << (lw_i64 v)                   
        {   char buffer [128];
            sprintf (buffer, lw_fmt_size, v);
            Add (buffer, -1);
            return *this;                                   
        }                                                   

        inline void Reset()
        {
            Size = Offset = 0;
        }

        inline void Write
            (Client &Socket, int offset = 0)
        {
            Socket.Write (Buffer + Offset, Size - Offset);
        }

        inline void Write
            (Lacewing::Server::Client &Socket, int offset = 0)
        {
            Socket.Write (Buffer + Offset, Size - Offset);
        }

        inline void Write
            (Lacewing::UDP &UDP, Lacewing::Address &Address, int offset = 0)
        {
            UDP.Write (Address, Buffer + Offset, Size - Offset);
        }
    };
}

#endif

