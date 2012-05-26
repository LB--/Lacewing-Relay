
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

#ifndef LacewingUtility
#define LacewingUtility

/* TODO : The utility classes could definitely all do with improvement. */

namespace Lacewing
{
    template <class T> struct List
    {   
        struct Element
        {
            Element * Next, * Prev;
            T Value;
            
            inline T &operator * ()
            {
                return Value;
            }
            
        } * First, * Last;
        
        int Size;

        inline List ()
        {
            First = Last = 0;
            Size = 0;
        }
        
        inline ~ List ()
        {
            Clear ();
        }

        inline Element * Push (T What)
        {
            Element * E = Push ();
            E->Value = What;
            return E;
        }

        inline Element * Push ()
        {
            Element * E = new (std::nothrow) Element;
            
            if (!E)
                return 0;

            E->Next  = 0;
            E->Prev  = Last;
            
            ++ Size;
            
            if (Last)
            {
                Last->Next = E;
                Last = E;
            }
            else
            {
                First = Last = E;
            }

            return E;
        }

        inline Element * Find (T What)
        {
            for (Element * E = First; E; E = E->Next)
                if (** E == What)
                    return E;

            return 0;
        }

        inline void Remove (T What)
        {
            Element * E = Find (What);

            if (E)
                Erase (E);
        }

        inline Element * PushFront (T What)
        {
            Element * E = PushFront ();
            E->Value = What;
            return E;
        }

        inline Element * PushFront ()
        {
            return Size > 0 ? InsertBefore (First) : Push ();
        }

        inline Element * InsertBefore (Element * Before, T What)
        {
            Element * E = InsertBefore (Before);
            E->Value = What;
            return E;
        }

        inline Element * InsertBefore (Element * Before)
        {
            Element * E = new (std::nothrow) Element;

            if (!E)
                return 0;

            if (Before->Prev)
                Before->Prev->Next = E;

            E->Prev = Before->Prev;
            Before->Prev = E;

            E->Next  = Before;

            if (Before == First)
                First = E;

            ++ Size;

            return E;
        }

        inline void Pop ()
        {
            Erase (Last);
        }

        inline void PopFront ()
        {
            Erase (First);
        }

        inline Element * Erase (Element * E)
        {
            Element * next = E->Next;

            if (E == First)
                First = E->Next;

            if (E == Last)
                Last = E->Prev;

            if (E->Next)
                E->Next->Prev = E->Prev;

            if (E->Prev)
                E->Prev->Next = E->Next;
        
            delete E;

            -- Size;
            
            LacewingAssert (Size >= 0);

            return next;
        }
        
        inline void Clear ()
        {
            while (First)
            {
                Element * Next = First->Next;
                delete First;
                First = Next;
            }

            Last = 0;
            Size = 0;
        }

    };

    template <class T> struct Array
    {       
        T * Items;
        
        int Allocated, Size;

        inline Array ()
        {
            Items = (T *) malloc (sizeof (T) * (Allocated = 16));
            Size = 0;
        }
        
        inline ~Array ()
        {
            free (Items);
        }

        inline void Push (T What)
        {
            if (Size + 1 >= Allocated)
                Items = (T *) realloc (Items, sizeof (T) * (Allocated *= 3));

            Items [Size ++] = What;
        }

        inline T Pop ()
        {
            return Items [-- Size];
        }

        inline T operator [] (int Index)
        {
            return Items [Index];
        }

        inline operator T * ()
        {
            return Items;
        }

        inline void Clear ()
        {
            Size = 0;
        }
    };
}

#endif


