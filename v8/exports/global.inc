
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


