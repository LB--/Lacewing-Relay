
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
    
    void webserver_onGet(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
    {
        Persistent <Function> Callback = ((CallbackInfo *) Webserver.Tag)->Callback;
        Local <Value> args [2];
        
        args[0] = String::New("get");
        args[1] = External::New(&Request);
        
        Callback->Call(Callback, 2, args);
    }

    void webserver_onPost(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
    {
        Persistent <Function> Callback = ((CallbackInfo *) Webserver.Tag)->Callback;
        Local <Value> args [2];
        
        args[0] = String::New("post");
        args[1] = External::New(&Request);
        
        Callback->Call(Callback, 2, args);
    }

    void webserver_onHead(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
    {
        Persistent <Function> Callback = ((CallbackInfo *) Webserver.Tag)->Callback;
        Local <Value> args [2];
        
        args[0] = String::New("head");
        args[1] = External::New(&Request);
        
        Callback->Call(Callback, 2, args);
    }

    void webserver_onError(Lacewing::Webserver &Webserver, Lacewing::Error &Error)
    {
        Persistent <Function> Callback = ((CallbackInfo *) Webserver.Tag)->Callback;
        Local <Value> args [2];
        
        args[0] = String::New("error");
        args[1] = External::New(&Error);
        
        Callback->Call(Callback, 2, args);
    }

    void webserver_onConnect(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
    {
        Persistent <Function> Callback = ((CallbackInfo *) Webserver.Tag)->Callback;
        Local <Value> args [2];
        
        args[0] = String::New("connect");
        args[1] = External::New(&Request);
        
        Callback->Call(Callback, 2, args);
    }

    void webserver_onDisconnect(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
    {
        Persistent <Function> Callback = ((CallbackInfo *) Webserver.Tag)->Callback;
        Local <Value> args [2];
        
        args[0] = String::New("disconnect");
        args[1] = External::New(&Request);
        
        Callback->Call(Callback, 2, args);
    }

    void webserver_onUploadStart(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload &Upload)
    {
    }

    void webserver_onUploadChunk(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload &Upload, const char * Data, int Size)
    {
    }
     
    void webserver_onUploadDone(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload &Upload)
    {
    }

    void webserver_onUploadPost(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload * Uploads[], int UploadCount)
    {
    }

#endif

#define BeginExport() BeginExportGlobal(); \
    Read_Reference(Lacewing::Webserver, Webserver);

Export(lwjs_ws_new)
    #ifdef ExportBodies
    {
        BeginExportGlobal();
        
        Read_Reference(Lacewing::EventPump, EventPump);
        Read_Function(Callback);
        
        Lacewing::Webserver &Webserver = *new Lacewing::Webserver(EventPump);
        Webserver.Tag = new CallbackInfo (Persistent<Function>::New (Callback));
        
        Webserver.onGet (webserver_onGet);
        Webserver.onPost (webserver_onPost);
        Webserver.onHead (webserver_onHead);
        Webserver.onError (webserver_onError);
        Webserver.onConnect (webserver_onConnect);
        Webserver.onDisconnect (webserver_onDisconnect);
        Webserver.onUploadStart (webserver_onUploadStart);
        Webserver.onUploadChunk (webserver_onUploadChunk);
        Webserver.onUploadDone (webserver_onUploadDone);
        Webserver.onUploadPost (webserver_onUploadPost);
        
        Return_Ptr(&Webserver);
    }
    #endif
Export(lwjs_ws_delete)
    #ifdef ExportBodies
    {
        BeginExport();
        delete &Webserver;
        Return();
    }
    #endif
Export(lwjs_ws_host)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Int(Port);
        Webserver.Host(Port);
        Return();
    }
    #endif
Export(lwjs_ws_host_secure)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Int(Port);
        Webserver.HostSecure(Port);
        Return();
    }
    #endif
Export(lwjs_ws_host_filter)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Reference(Lacewing::Filter, Filter);
        Webserver.Host(Filter);
        Return();
    }
    #endif
Export(lwjs_ws_host_secure_filter)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Reference(Lacewing::Filter, Filter);
        Webserver.HostSecure(Filter);
        Return();
    }
    #endif
Export(lwjs_ws_unhost)
    #ifdef ExportBodies
    {
        BeginExport();
        Webserver.Unhost();
        Return();
    }
    #endif
Export(lwjs_ws_unhost_secure)
    #ifdef ExportBodies
    {
        BeginExport();
        Webserver.UnhostSecure();
        Return();
    }
    #endif
Export(lwjs_ws_hosting)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Bool(Webserver.Hosting());
    }
    #endif
Export(lwjs_ws_hosting_secure)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Bool(Webserver.HostingSecure());
    }
    #endif
Export(lwjs_ws_port)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Int(Webserver.Port());
    }
    #endif
Export(lwjs_ws_port_secure)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Int(Webserver.SecurePort());
    }
    #endif
Export(lwjs_ws_load_cert_file)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Filename);
        Read_String(Passphrase);
        Return_Bool(Webserver.LoadCertificateFile(Filename, Passphrase));
    }
    #endif
Export(lwjs_ws_load_sys_cert)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(StoreName);
        Read_String(CommonName);
        Read_String(Location);
        Return_Bool(Webserver.LoadSystemCertificate(StoreName, CommonName, Location));
    }
    #endif
Export(lwjs_ws_cert_loaded)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Bool(Webserver.CertificateLoaded());
    }
    #endif
Export(lwjs_ws_bytes_sent)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Int64(Webserver.BytesSent());
    }
    #endif
Export(lwjs_ws_bytes_received)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Int64(Webserver.BytesReceived());
    }
    #endif
Export(lwjs_ws_close_session)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Session);
        Webserver.CloseSession(Session);
        Return();
    }
    #endif
Export(lwjs_ws_enable_manual_finish)
    #ifdef ExportBodies
    {
        BeginExport();
        Webserver.EnableManualRequestFinish();
        Return();
    }
    #endif
    
#undef BeginExport

#define BeginExport() BeginExportGlobal(); \
    Read_Reference(Lacewing::Webserver::Request, Request);

Export(lwjs_ws_req_addr)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Ptr(&Request.GetAddress());
    }
    #endif
Export(lwjs_ws_req_secure)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Bool(Request.Secure());
    }
    #endif
Export(lwjs_ws_req_url)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_String(Request.URL());
    }
    #endif
Export(lwjs_ws_req_hostname)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_String(Request.Hostname());
    }
    #endif
Export(lwjs_ws_req_disconnect)
    #ifdef ExportBodies
    {
        BeginExport();
        Request.Disconnect();
        Return();
    }
    #endif 
Export(lwjs_ws_req_set_redirect)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(URL);
        Request.SetRedirect(URL);
        Return();
    }
    #endif
Export(lwjs_ws_req_set_response_type)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Int(Code);
        Read_String(Message);
        Request.SetResponseType(Code, Message);
        Return();
    }
    #endif
Export(lwjs_ws_req_set_mime_type)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(MimeType);
        Request.SetMimeType(MimeType);
        Return();
    }
    #endif
Export(lwjs_ws_req_guess_mime_type)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(MimeType);
        Request.GuessMimeType(MimeType);
        Return();
    }
    #endif
Export(lwjs_ws_req_set_charset)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Charset);
        Request.SetCharset(Charset);
        Return();
    }
    #endif
Export(lwjs_ws_req_send_text)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Text)
        Request.Send(Text);
        Return();
    }
    #endif
Export(lwjs_ws_req_sendfile)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Filename);
        Request.SendFile(Filename);
        Return();
    }
    #endif
Export(lwjs_ws_req_reset)
    #ifdef ExportBodies
    {
        BeginExport();
        Request.Reset();
        Return();
    }
    #endif
Export(lwjs_ws_req_finish)
    #ifdef ExportBodies
    {
        BeginExport();
        Request.Finish();
        Return();
    }
    #endif
Export(lwjs_ws_req_last_modified)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_Int64(Request.LastModified());
    }
    #endif
Export(lwjs_ws_req_set_last_modified)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_Int64(LastModified);
        Request.LastModified(LastModified);
        Return();
    }
    #endif
Export(lwjs_ws_req_set_unmodified)
    #ifdef ExportBodies
    {
        BeginExport();
        Request.SetUnmodified();
        Return();
    }
    #endif
Export(lwjs_ws_req_set_header)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Read_String(Val);
        Request.Header(Name, Val);
        Return();
    }
    #endif
Export(lwjs_ws_req_header)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Return_String(Request.Header(Name));
    }
    #endif
Export(lwjs_ws_req_set_cookie)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Read_String(Val);
        Request.Cookie(Name, Val);
        Return();
    }
    #endif
Export(lwjs_ws_req_cookie)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Return_String(Request.Cookie(Name));
    }
    #endif
Export(lwjs_ws_req_session_id)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_String(Request.Session());
    }
    #endif
Export(lwjs_ws_req_session_write)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Read_String(Val);
        Request.Session(Name, Val);
        Return();
    }
    #endif
Export(lwjs_ws_req_session_read)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Return_String(Request.Session(Name));
    }
    #endif
Export(lwjs_ws_req_session_close)
    #ifdef ExportBodies
    {
        BeginExport();
        Request.CloseSession();
        Return();
    }
    #endif
Export(lwjs_ws_req_GET)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Request.GET(Name);
        Return();
    }
    #endif
Export(lwjs_ws_req_POST)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Request.POST(Name);
        Return();
    }
    #endif
Export(lwjs_ws_req_disable_cache)
    #ifdef ExportBodies
    {
        BeginExport();
        Request.DisableCache();
        Return();
    }
    #endif
    
#undef BeginExport

#define BeginExport() BeginExportGlobal(); \
    Read_Reference(Lacewing::Webserver::Upload, Upload);

Export(lwjs_ws_upload_form_el_name)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_String(Upload.FormElementName());
    }
    #endif
Export(lwjs_ws_upload_filename)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_String(Upload.Filename());
    }
    #endif
Export(lwjs_ws_upload_header)
    #ifdef ExportBodies
    {
        BeginExport();
        Read_String(Name);
        Return_String(Upload.Header(Name));
    }
    #endif
Export(lwjs_ws_upload_set_autosave)
    #ifdef ExportBodies
    {
        BeginExport();
        Upload.SetAutoSave();
        Return();
    }
    #endif
Export(lwjs_ws_upload_autosave_fname)
    #ifdef ExportBodies
    {
        BeginExport();
        Return_String(Upload.GetAutoSaveFilename());
    }
    #endif
    
#undef BeginExport

