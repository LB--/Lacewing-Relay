
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

#define LooperStruct(Name, It) \
struct Name { Name(Lacewing::Sync::Lock * L, Lacewing::SpinSync::ReadLock * RL, It Iterator) \
    { Lock = L; SpinLock = RL; this->Iterator = Iterator; } ~Name() { delete Lock; delete SpinLock; } \
    Lacewing::Sync::Lock * Lock; Lacewing::SpinSync::ReadLock * SpinLock; It Iterator; };

#define LooperSync(InternalClass, SyncObject) \
    L = new Lacewing::Sync::Lock(((InternalClass *)InternalTag)->SyncObject)

#define LooperSpinSync(InternalClass, SyncObject) \
    RL = new Lacewing::SpinSync::ReadLock(((InternalClass *)InternalTag)->SyncObject)

#define Looper(X, Class, LoopName, InternalClass, Container, LockCode, UnlockCode, IteratorA, PublicType, Public) \
    LooperStruct(X##LoopName##Looper, IteratorA); \
void * Lacewing::Class::LoopName##Loop(void * ID) { \
    if(!ID) \
{ \
        Lacewing::Sync::Lock * L = 0;\
        Lacewing::SpinSync::ReadLock * RL = 0;\
        LockCode;\
        if(((InternalClass *)InternalTag)->Container.begin() == ((InternalClass *)InternalTag)->Container.end())\
        {\
            UnlockCode;\
            delete L;\
            delete RL;\
            return 0;\
        }\
        IteratorA Begin = ((InternalClass *)InternalTag)->Container.begin();\
        return (void *) new X##LoopName##Looper(L, RL, Begin);\
    }\
    else\
    {\
        X##LoopName##Looper * Looper = (X##LoopName##Looper *)ID;\
        if(Looper->Iterator == ((InternalClass *)InternalTag)->Container.end())\
        {\
            UnlockCode;\
            delete Looper;\
            return 0;\
        }\
        ++Looper->Iterator;\
        if(Looper->Iterator != ((InternalClass *)InternalTag)->Container.end())\
            return (void *)Looper;\
        UnlockCode;\
        delete Looper;\
        return 0;\
    }\
} \
void Lacewing::Class::End##LoopName##Loop(void * ID) { UnlockCode; delete (X##LoopName##Looper *) ID; } \
PublicType Lacewing::Class::LoopName##LoopIndex(void * ID)\
{ return (*((X##LoopName##Looper *)ID)->Iterator)Public; }

