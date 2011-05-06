
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

#ifndef LacewingIDPool
#define LacewingIDPool

class IDPool
{
    
protected:

    set <unsigned short> IDs;

    unsigned short NextID;
    int BorrowedCount;

public:

    inline IDPool()
    {
        NextID        = 0;
        BorrowedCount = 0;
    }
    
    inline unsigned short Borrow()
    {
        ++ BorrowedCount;

        if(IDs.size())
        {
            unsigned short ID = *IDs.begin();
            IDs.erase(IDs.begin());

            return ID;
        }
        else
            return NextID ++;
    }

    inline void Return(unsigned short ID)
    {
        if((-- BorrowedCount) == 0)
        {
            IDs.clear();
            NextID = 0;
        }
        else
            IDs.insert(ID);
    }
};

#endif

