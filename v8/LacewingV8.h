
/*
    Copyright (C) 2011 James McLaughlin

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE. 
*/

#include <Lacewing.h>
#include <v8.h>

namespace Lacewing
{
    namespace V8
    {
        /* If you don't pass a Pump to Export, Lacewing.EventPump will be
           exported and the Javascript will have to create an EventPump,
           construct the Lacewing classes with it, call startEventLoop() or
           tick(), etc ...
           
           If you do pass a Pump, Lacewing.EventPump won't be exported to the
           Javascript, and that Pump will be used whenever a Lacewing class is
           to be constructed.  This makes it possible to eg. use the existing
           libev loop in Node.JS.
        */
        
        void Export (v8::Handle<v8::Context> Context);
        void Export (v8::Handle<v8::Context> Context, Lacewing::Pump &);
        
        void Export (v8::Handle<v8::Object> Object);
        void Export (v8::Handle<v8::Object> Object, Lacewing::Pump &);
    }
}

