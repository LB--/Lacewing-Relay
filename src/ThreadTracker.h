
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

#ifndef LacewingThreadTracker
#define LacewingThreadTracker    

class ThreadTracker
{
    
protected:

    volatile long Count;

public:

    bool Dying;

    inline ThreadTracker()
    {
        Count = 0;
        Dying = false;
    }

    inline ~ThreadTracker()
    {
        LacewingAssert(Count == 0);
    }

    struct StartInfo
    {
        void * Parameter;
        ThreadTracker * Tracker;
    };

    inline void Start(void * Function, void * Parameter)
    {
        LacewingSyncIncrement(&Count);

        StartInfo * Info = new StartInfo;

        Info->Tracker   = this;
        Info->Parameter = Parameter;

        Lacewing::StartThread(Function, Info);
    }

    inline bool Living()
    {
        return Count > 0;
    }

    inline void WaitUntilDead()
    {
        Dying = true;

        while(Count > 0)
            LacewingYield();
    }


    /** Never to be called explicitly, only by the macro below **/

    inline void ThreadEnding()
    {   LacewingSyncDecrement(&Count);
    }
};

#define LacewingThread(Name, ParameterType, ParameterName) \
    void T##Name(ParameterType &);                         \
    void Name(ThreadTracker::StartInfo * _Info)            \
    {                                                      \
        ThreadTracker::StartInfo Info(*_Info);             \
        delete _Info;                                      \
                                                           \
        Lacewing::SetCurrentThreadName(#Name);             \
        T##Name(*(ParameterType *) Info.Parameter);        \
        Info.Tracker->ThreadEnding();                      \
    }                                                      \
    void T##Name(ParameterType &ParameterName)              
    

#endif

