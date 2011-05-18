
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

