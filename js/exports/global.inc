
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

Export(lwjs_version)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Return_String(Lacewing::Version());
    }
    #endif
Export(lwjs_file_last_modified)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Read_String(Filename);
        Return_Int64(Lacewing::LastModified(Filename));
    }
    #endif
Export(lwjs_file_exists)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Read_String(Filename);
        Return_Bool(Lacewing::FileExists(Filename));
    }
    #endif
Export(lwjs_file_size)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Read_String(Filename);
        Return_Int64(Lacewing::FileSize(Filename));
    }
    #endif
Export(lwjs_temp_path)
    #ifdef ExportBodies
    {
        BeginExportGlobal();

        char Path[2048];
        Lacewing::TempPath(Path, sizeof(Path));
        Return_String(Path);
    }
    #endif
Export(lwjs_new_temp_file)
    #ifdef ExportBodies
    {
        BeginExportGlobal();

        char Path[2048];
        Lacewing::NewTempFile(Path, sizeof(Path));
        Return_String(Path);
    }
    #endif
Export(lwjs_guess_mime_type)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Read_String(Filename);
        Return_String(Lacewing::GuessMimeType(Filename));
    }
    #endif


