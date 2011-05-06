
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
            if(!stricmp(Current->Key, Key))
                return Current->Value;
        
        return "";
    }

    inline const char * Set(const char * Key, const char * Value)
    {
        for(Item * Current = First; Current; Current = Current->Next)
        {
            if(!stricmp(Current->Key, Key))
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
            if(!stricmp(Current->Key, Key))
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

