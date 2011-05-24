
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

Deleter (Filter);
    
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


