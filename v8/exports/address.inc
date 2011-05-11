
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


