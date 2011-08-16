
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

struct Map
{
public:

    struct Item
    {
        char * Key, * Value;
        Item * Next;
    };

    Item * First;

    inline Map()
    {
        First = 0;
    }

    inline ~Map()
    {
        Clear();
    }

    inline const char * Get(const char * Key)
    {
        for(Item * Current = First; Current; Current = Current->Next)
            if(!strcasecmp(Current->Key, Key))
                return Current->Value;
        
        return "";
    }

    inline const char * Set(const char * Key, const char * Value)
    {
        for(Item * Current = First; Current; Current = Current->Next)
        {
            if(!strcasecmp(Current->Key, Key))
            {
                free(Current->Value);
                Current->Value = strdup(Value);

                return Current->Value;
            }
        }
        
        Item * New = new Item;
        
        New->Key = strdup(Key);
        New->Value = strdup(Value);
        New->Next = First;

        First = New;

        return New->Value;
    }

    inline void SetAndGC(char * Key, char * Value)
    {
        for(Item * Current = First; Current; Current = Current->Next)
        {
            if(!strcasecmp(Current->Key, Key))
            {
                free(Current->Value);
                Current->Value = Value;

                free(Key);
                return;
            }
        }
        
        Item * New = new Item;
        
        New->Key = Key;
        New->Value = Value;
        New->Next = First;

        First = New;
    }

    inline void Clear()
    {
        while(First)
        {
            free(First->Key);
            free(First->Value);

            Item * Next = First->Next;

            delete First;
            First = Next;
        }
    }
};

