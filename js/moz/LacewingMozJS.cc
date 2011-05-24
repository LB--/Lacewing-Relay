
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

#include "LacewingMozJS.h"
    
#define BeginExportGlobal() int _arg_index = 2;

#define Get_Argument() argv[_arg_index ++]
#define Get_Pointer() JS_GetPrivate(Context, JSVAL_TO_OBJECT(Get_Argument()))

#define Read_Reference(T, N) T &N = *(T *) Get_Pointer();
#define Read_Int(N) int N = JSVAL_TO_INT(Get_Argument());
#define Read_Int64(N) lw_i64 N = JSVAL_TO_INT(Get_Argument());
#define Read_Bool(N) bool N = JSVAL_TO_BOOLEAN(Get_Argument());
#define Read_String(N) const char * N; { const jschar * _str = JS_GetStringCharsZ(Context, JSVAL_TO_STRING(Get_Argument())); \
                            N = js_DeflateString(Context, _str, js_strlen(_str)); }
                            
#define Read_Function(N) jsval N = Get_Argument();

#define Return() return JS_TRUE;
#define Return_Bool(x) JS_SET_RVAL(Context, argv, x ? JSVAL_TRUE : JSVAL_FALSE); return JS_TRUE;
#define Return_String(x) JS_SET_RVAL(Context, argv, STRING_TO_JSVAL(JS_NewStringCopyZ(Context, x))); return JS_TRUE;
#define Return_Int(x) JS_SET_RVAL(Context, argv, INT_TO_JSVAL(x)); return JS_TRUE;
#define Return_Int64(x) JS_SET_RVAL(Context, argv, INT_TO_JSVAL(x)); return JS_TRUE;
#define Return_Ref(x) JS_SET_RVAL(Context, argv, MakeRef (Context, &x)); return JS_TRUE;
#define Return_New(x, c) JS_SET_RVAL(Context, argv, MakeRef (Context, x, c)); return JS_TRUE;

#define CALLBACK_INIT() JSContext * Context = ((CallbackInfo *) Webserver.Tag)->Context;
#define CALLBACK_TYPE jsval
#define CALLBACK_ARG_TYPE jsval
#define CALLBACK_ARG_STRING(x) STRING_TO_JSVAL(JS_NewStringCopyZ(Context, x))
#define CALLBACK_ARG_REF(x) MakeRef (Context, &x)
#define CALLBACK_DO(callback, argc, argv) \
    do { jsval rval; JS_CallFunctionValue (Context, JS_GetGlobalObject(Context), callback, argc, argv, &rval); } while(0);
#define CALLBACK_INFO(c) new CallbackInfo (c, Context)

struct CallbackInfo
{
	CallbackInfo (CALLBACK_TYPE _Callback, JSContext * _Context)
        : Callback(_Callback), Context(_Context)
	{
        JS_AddValueRoot(Context, &Callback);
	}

	CALLBACK_TYPE Callback;
    JSContext * Context;
};

JSClass HasPrivate =
{
    "lwref", JSCLASS_HAS_PRIVATE | JSCLASS_CONSTRUCT_PROTOTYPE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

jsval MakeRef (JSContext * Context, void * ptr)
{
    JSObject * Ref = JS_NewObject (Context, &HasPrivate, 0, JS_GetGlobalObject(Context));
    JS_SetPrivate (Context, Ref, ptr);
    return OBJECT_TO_JSVAL (Ref);
}

jsval MakeRef (JSContext * Context, void * ptr, void * deleter)
{   
    /* TODO */

    return MakeRef (Context, ptr);
}

#define ExportBodies
#define Export(x) JSBool x (JSContext * Context, uintN argc, jsval * argv)
#define Deleter(x) void * x##Deleter = 0; /* TODO */

    #include "../exports/eventpump.inc"
    #include "../exports/global.inc"
    #include "../exports/webserver.inc"
    #include "../exports/address.inc"
    #include "../exports/error.inc"
    #include "../exports/filter.inc"

#undef Deleter
#undef ExportBodies
#undef Export

#include "../js/liblacewing.js.inc"

void Lacewing::MozJS::Export(JSContext * Context, JSObject * Target, Lacewing::Pump * Pump)
{
    JSObject * Exports = JS_NewObject(Context, 0, 0, JS_GetGlobalObject(Context));
    
    #define Export(x) { JSFunction * fn = JS_NewFunction(Context, x, 0, 0, JS_GetGlobalObject(Context), #x); \
                            jsval _fn = OBJECT_TO_JSVAL((JSObject *) fn); \
                            JS_SetProperty(Context, Exports, #x, &_fn); }
    #define Deleter(x)
    
        if(!Pump)
        {
            #include "../exports/eventpump.inc"
        }
        else
        {
            jsval ep = MakeRef(Context, Pump);
            JS_GetPrivate(Context, JSVAL_TO_OBJECT(ep));
            JS_SetProperty(Context, Exports, "lwjs_global_eventpump", &ep);
        }

        #include "../exports/global.inc"
        #include "../exports/webserver.inc"
        #include "../exports/address.inc"
        #include "../exports/error.inc"
        #include "../exports/filter.inc"
    
    #undef Deleter
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

void Lacewing::MozJS::Export(JSContext * Context, Lacewing::Pump * Pump)
{    
    JSObject * Target = JS_NewObject(Context, 0, 0, JS_GetGlobalObject(Context));
    Export (Context, Target, Pump);
    
    jsval _Target = OBJECT_TO_JSVAL(Target);
    JS_SetProperty(Context, JS_GetGlobalObject(Context), "Lacewing", &_Target);
}
