
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

    void AddressDeleter (Persistent<Value> Ref, void *)
    {
        printf("Deleting Address!\n");
        
        delete ((Lacewing::Address *) External::Unwrap(Object::Cast(*Ref)->GetInternalField(0)));
    }
    
#endif
    
#define BeginExport() BeginExportGlobal(); \
    Read_Reference(Lacewing::Address, Address);

Export(lwjs_address_new)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Return_New(new Lacewing::Address(), AddressDeleter);
    }
    #endif
Export(lwjs_address_new_name)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Read_String(Name)
        Read_Int(Port)
        Return_New(new Lacewing::Address(Name, Port, false), AddressDeleter);
    }
    #endif
Export(lwjs_address_new_name_blocking)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Read_String(Name)
        Read_Int(Port)
        Return_New(new Lacewing::Address(Name, Port, true), AddressDeleter);
    }
    #endif
Export(lwjs_address_copy)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_New(new Lacewing::Address(Address), AddressDeleter);
    }
    #endif
Export(lwjs_address_delete)
    #ifdef ExportBodies
    {
        BeginExport();
        delete &Address;
        Return();
    }
    #endif
Export(lwjs_address_port)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Int(Address.Port());
    }
    #endif
Export(lwjs_address_set_port)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Int(Port);
        Address.Port(Port);
        Return();
    }
    #endif
Export(lwjs_address_is_ready)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Bool(Address.Ready());
    }
    #endif
Export(lwjs_address_tostring)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_String(Address.ToString());
    }
    #endif
    
#undef BeginExport


