
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

#include "LacewingV8.h"
using namespace v8;

struct CallbackInfo
{
	CallbackInfo (Persistent<Function> _Callback) : Callback(_Callback)
	{
	}
	
	Persistent<Function> Callback;
};
    
#define BeginExportGlobal() int _arg_index = 0;

#define Get_Argument() args[_arg_index ++]
#define Get_Pointer() External::Unwrap(Get_Argument()->ToObject()->GetInternalField(0))

#define Read_Reference(T, N) T &N = *(T *) Get_Pointer();
#define Read_Int(N) int N = Get_Argument()->ToInt32()->Int32Value();
#define Read_Int64(N) int N = Get_Argument()->ToInteger()->IntegerValue();
#define Read_Bool(N) bool N = Get_Argument()->ToInt32()->BooleanValue();
#define Read_String(N) String::AsciiValue _##N(Get_Argument()->ToString()); \
                        const char * N = *_##N;
#define Read_Function(N) Handle<Function> N = Handle<Function> (Function::Cast(*Get_Argument()));

#define Return() return Handle<Value>();
#define Return_Bool(x) return Boolean::New(x);
#define Return_String(x) return String::New(x);
#define Return_Int(x) return Int32::New(x);
#define Return_Int64(x) return Integer::New(x);
#define Return_Ref(x) return MakeRef (&x);
#define Return_New(x, c) return MakeRef (x, c);

Persistent <FunctionTemplate> RefTemplate;

Handle <Value> RefConstructor(const Arguments &)
{
    return Handle <Value> ();
}

Handle <Object> MakeRef (void * ptr, WeakReferenceCallback delete_callback)
{
    HandleScope Scope;
    Persistent <Object> Ref = Persistent <Object>::New (RefTemplate->GetFunction()->NewInstance());
   
    Ref->SetInternalField(0, External::New(ptr));
    Ref.MakeWeak(0, delete_callback);
    
    return Ref;
}

Handle <Object> MakeRef (void * ptr)
{
    HandleScope Scope;

    Handle <Object> Ref = RefTemplate->GetFunction()->NewInstance();
    Ref->SetInternalField(0, External::New(ptr));
   
    return Scope.Close(Ref);
}

Local <Value> MakeRefLocal (void * ptr)
{
    return Local<Value>::New(MakeRef(ptr));
}

#define ExportBodies
#define Export(x) Handle<Value> x (const Arguments &args)

    #include "exports/eventpump.inc"
    #include "exports/global.inc"
    #include "exports/webserver.inc"
    #include "exports/address.inc"
    #include "exports/error.inc"
    #include "exports/filter.inc"
    
#undef ExportBodies
#undef Export

#include "js/liblacewing.js.inc"

Handle<Function> Lacewing::V8::Create()
{
    HandleScope Scope;

    RefTemplate = Persistent <FunctionTemplate>::New (FunctionTemplate::New(RefConstructor));
    Handle <ObjectTemplate> RefInstTemplate = RefTemplate->InstanceTemplate();
    RefInstTemplate->SetInternalFieldCount(1);

    Handle <Object> Exports = Object::New();
    
    #define Export(x) Handle <FunctionTemplate> x##_template = FunctionTemplate::New(x); \
                Exports->Set(String::New(#x), x##_template->GetFunction());
        
        #include "exports/eventpump.inc"
        #include "exports/global.inc"
        #include "exports/webserver.inc"
        #include "exports/address.inc"
        #include "exports/error.inc"
        #include "exports/filter.inc"
    
    #undef Export
        
    Local<Function> function = Function::Cast(*Script::Compile(
            String::New((const char *) LacewingJS, (int) sizeof(LacewingJS)),
            String::New("liblacewing.js"))->Run());

    Handle<Value> ExportsParam = Exports;
    return Scope.Close(Handle<Function> (Function::Cast(*function->Call(function, 1, &ExportsParam))));
}

void Lacewing::V8::Export(Handle<Context> Context)
{
    Context->Global()->Set(String::New("Lacewing"), Lacewing::V8::Create());
}

