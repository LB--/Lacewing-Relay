
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

template<class P, class T> class Backlog
{
protected:

    unsigned int Count;
    vector<T *> List;

    Lacewing::SpinSync Sync;

public:

    inline Backlog()
    {
        Count = 8;
    }

    inline Backlog(unsigned int Count)
    {
        this->Count = Count;   
    }

    inline ~Backlog()
    {
        for(typename vector<T *>::iterator it = List.begin(); it != List.end(); ++ it)
        {
            #ifdef LacewingWindows
            #ifdef LacewingDebug

                DWORD Old;

                VirtualProtect(*it, sizeof(T), PAGE_READWRITE, &Old);
                VirtualFree(*it, MEM_RELEASE, 0);

                continue;

            #endif
            #endif

            free(*it);
        }
    }

    inline T &Borrow(P &Parent)
    {
        T * Borrowed;

        {   Lacewing::SpinSync::WriteLock Lock(Sync);

            if(!List.size())
            {
                for(unsigned int i = Count; i; -- i)
                {
                    #ifdef LacewingDebug
                    #ifdef LacewingWindows
        
                        List.push_back((T *) VirtualAlloc(0, sizeof(T), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
                        continue;

                    #endif
                    #endif
            
                    List.push_back((T *) malloc(sizeof(T)));
                }
            }

            Borrowed = List.back();
            List.pop_back();
        }

        #ifdef LacewingDebug
        #ifdef LacewingWindows

            /* Restore normal protection (see Return) */

            DWORD Old;
            VirtualProtect(Borrowed, sizeof(T), PAGE_READWRITE, &Old);

        #endif
        #endif

        return * new (Borrowed) T (Parent);
    }

    inline void Return(T &Object)
    {
        Object.~T();

        #ifdef LacewingDebug

            memset(&Object, 0, sizeof(T));

            #ifdef LacewingWindows

                /* Protect the memory so that an exception is thrown if something is used after
                   being returned to the backlog */

                DWORD Old;
                VirtualProtect(&Object, sizeof(T), PAGE_NOACCESS, &Old);

            #endif

        #endif

        Lacewing::SpinSync::WriteLock Lock(Sync);
        List.push_back(&Object);
    }


};

