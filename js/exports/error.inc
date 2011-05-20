
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
    
#ifdef ExportBodies

    void ErrorDeleter (Persistent<Value> Ref, void *)
    {
        printf("Deleting Error!\n");
        
        delete ((Lacewing::Error *) External::Unwrap(Object::Cast(*Ref)->GetInternalField(0)));
    }
    
#endif

#define BeginExport() BeginExportGlobal(); \
    Read_Reference(Lacewing::Error, Error);

Export(lwjs_error_new)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Return_New(new Lacewing::Error(), ErrorDeleter);
    }
    #endif
Export(lwjs_error_delete)
    #ifdef ExportBodies
    {
        BeginExport();
        delete &Error;
        Return();
    }
    #endif
Export(lwjs_error_add)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Text);
        Error.Add(Text);
        Return();
    }
    #endif
Export(lwjs_error_tostring)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_String(Error.ToString());
    }
    #endif
Export(lwjs_error_clone)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_New(new Lacewing::Error(Error), ErrorDeleter);
    }
    #endif
    
#undef BeginExport

