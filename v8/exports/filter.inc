
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

    void FilterDeleter (Persistent<Value> Ref, void *)
    {
        delete ((Lacewing::Filter *) External::Unwrap(Object::Cast(*Ref)->GetInternalField(0)));
    }
    
#endif
    
#define BeginExport() BeginExportGlobal(); \
    Read_Reference(Lacewing::Filter, Filter);

Export(lwjs_filter_new)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Return_New(new Lacewing::Filter(), FilterDeleter);
    }
    #endif
Export(lwjs_filter_delete)
    #ifdef ExportBodies
    {
        BeginExport();
        delete &Filter;
        Return();
    }
    #endif
Export(lwjs_filter_set_local_ip)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Int(IP);
        Filter.LocalIP(IP);
        Return();
    }
    #endif
Export(lwjs_filter_set_local)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Filter.Local(Name);
        Return();
    }
    #endif
Export(lwjs_filter_set_remote)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Filter.Remote(Name);
        Return();
    }
    #endif
Export(lwjs_filter_set_remote_addr)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Reference(Lacewing::Address, Address);
        Filter.Remote(Address);
        Return();
    }
    #endif
Export(lwjs_filter_get_remote_addr)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Ref(Filter.Remote());
    }
    #endif
Export(lwjs_filter_set_reuse)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Bool(Enabled);
        Filter.Reuse(Enabled);
        Return();
    }
    #endif
Export(lwjs_filter_is_reuse_set)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Bool(Filter.Reuse());
    }
    #endif  
    
#undef BeginExport


