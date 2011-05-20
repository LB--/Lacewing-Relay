
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

#include "LaceMonkey.h"
    
#define BeginExportGlobal() int _arg_index = 0;

#define Get_Argument() argv[_arg_index ++]
#define Get_Pointer() External::Unwrap(Get_Argument()->ToObject()->GetInternalField(0))

#define Read_Reference(T, N) T &N = *(T *) Get_Pointer();
#define Read_Int(N) int N = JSVAL_TO_INT(Get_Argument());
#define Read_Int64(N) lw_i64 N = JSVAL_TO_INT(Get_Argument());
#define Read_Bool(N) bool N = JSVAL_TO_BOOLEAN(Get_Argument());
#define Read_String(N) const char * N; { jschar * _str = JS_GetStringCharsZ(Context, JSVAL_TO_STRING(Read_Argument())); \
                            N = js_DeflateString(Context, _str, js_strlen(_str)); }
                            
#define Read_Function(N) jsval N = Get_Argument();

#define Return() return JS_TRUE;
#define Return_Bool(x) JS_SET_RVAL(Context, argv, x ? JSVAL_TRUE : JSVAL_FALSE); return JS_TRUE;
#define Return_String(x) JS_SET_RVAL(Context, argv, STRING_TO_JSVAL(JS_NewStringCopyZ(Context, x))); return JS_TRUE;
#define Return_Int(x) JS_SET_RVAL(Context, argv, INT_TO_JSVAL(x)); return JS_TRUE;
#define Return_Int64(x) JS_SET_RVAL(Context, argv, INT_TO_JSVAL(x)); return JS_TRUE;
#define Return_Ref(x) return MakeRef (&x);
#define Return_New(x, c) return MakeRef (x, c);

#define ExportBodies
#define Export(x) JSBool x (JSContext * Context, uintN argc, jsval * argv)

    #include "exports/eventpump.inc"
    #include "exports/global.inc"
    #include "exports/webserver.inc"
    #include "exports/address.inc"
    #include "exports/error.inc"
    #include "exports/filter.inc"
    
#undef ExportBodies
#undef Export

#include "js/liblacewing.js.inc"

namespace Lacewing
{
    namespace V8
    {
        Lacewing::Pump * Pump = 0;
    }    
}

void Lacewing::V8::Export(JSObject * Target, Lacewing::Pump * Pump)
{
    HandleScope Scope;

    RefTemplate = Persistent <FunctionTemplate>::New (FunctionTemplate::New(RefConstructor));
    Handle <ObjectTemplate> RefInstTemplate = RefTemplate->InstanceTemplate();
    RefInstTemplate->SetInternalFieldCount(1);
    
    JSObject * Exports = JS_NewObject(Context, 0, 0, JS_GetGlobalObject(Context));
    
    #define Export(x) { JSFunction * fn = JS_NewFunction(Context, x, 0, 0, JS_GetGlobalObject(Context), #x); \
                            JS_SetProperty(Context, Exports, #x, fn); }

        if(!Pump)
        {
            #include "exports/eventpump.inc"
        }
        else
        {   Exports->Set(String::New("lwjs_global_eventpump"), MakeRef(Pump));
        }

        #include "exports/global.inc"
        #include "exports/webserver.inc"
        #include "exports/address.inc"
        #include "exports/error.inc"
        #include "exports/filter.inc"
    
    #undef Export
    
    JSObject * Script = JS_CompileScript (Context, JS_GetGlobalObject(Context),
                                (const char *) LacewingJS, strlen((const char *) LacewingJS),
                                     "liblacewing.js", 0);

    jsval rval = JSVAL_VOID;

    if(Script)
    {
        JS_ExecuteScript (Context, JS_GetGlobalObject(Context), Script, &rval);
        JS_RemoveObjectRoot (Context, &Script);
    }

    if(rval != JSVAL_VOID)
    {
        jsval params [] = { OBJECT_TO_JSVAL(Exports), OBJECT_TO_JSVAL(Target) };
        JS_CallFunctionValue (Context, JS_GetGlobalObject(Context), rval, 2, params, &rval);
    }
}

void Lacewing::V8::Export(JSContext * Context, Lacewing::Pump * Pump)
{    
    JSObject * Target = JS_NewObject(Context, 0, 0, JS_GetGlobalObject(Context));
    Export (Target);
    
    JS_SetProperty(Context, JS_GetGlobalObject(Context), "Lacewing", OBJECT_TO_JSVAL(Target));
}
