
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

    void EventPumpDeleter (Persistent<Value> Ref, void *)
    {
        printf("Deleting EventPump!\n");
        
        delete ((Lacewing::EventPump *) External::Unwrap(Object::Cast(*Ref)->GetInternalField(0)));
    }
    
#endif
    
#define BeginExport() BeginExportGlobal(); \
    Read_Reference(Lacewing::EventPump, EventPump);

Export(lwjs_eventpump_new)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        Return_New(new Lacewing::EventPump(), EventPumpDeleter);
    }
    #endif
Export(lwjs_eventpump_delete)
    #ifdef ExportBodies
    {
        BeginExport();
        delete &EventPump;
        Return();
    }
    #endif
Export(lwjs_eventpump_tick)
    #ifdef ExportBodies
    {
        BeginExport();
        EventPump.Tick();
        Return();
    }
    #endif
Export(lwjs_eventpump_start_event_loop)
    #ifdef ExportBodies
    {
        BeginExport();
        EventPump.StartEventLoop();
        Return();
    }
    #endif

#undef BeginExport

