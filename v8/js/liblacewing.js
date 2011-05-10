
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

(function(exports)
{
    var Lacewing = function()
    {   return Lacewing.version();
    },
    addBinders = function(c, callbacks, on)
    {   c.bind = function(ev, fn)
        {   callbacks[ev].push(fn);
            return c;
        }
        on = on.split(' ');
        for(var i in on)
        { (function() {
            var cb = on[i];
            callbacks [cb] = [];
            c [cb] = function(fn)
            { return c.bind(cb, fn);
            }
        }) () }
    };
      
    Lacewing.version = function()
    {  return exports.lwjs_version();
    };
    
    /*** EventPump ***/
    
    (Lacewing.EventPump = function()
    {            
       this._lw_ref = exports.lwjs_eventpump_new();
       return this;
       
    }).prototype =
    {
        tick: function()
        {   exports.lwjs_eventpump_tick(this._lw_ref);
            return this;
        },
        startEventLoop: function()
        {   exports.lwjs_eventpump_start_event_loop(this._lw_ref);
            return this;
        }
    };
    
    /*** Error ***/
    
    (Lacewing.Error = function(a, b)
    {            
       if(a === undefined)
           this._lw_ref = exports.lwjs_error_new();
       else if(a instanceof Lacewing.Error)
           this._lw_ref = exports.lwjs_error_clone(a._lw_ref);
       else if(a === exports)
           this._lw_ref = b;
       else throw "Invalid parameters";
           
       return this;
       
    }).prototype =
    {
        add: function(text)
        {   exports.lwjs_error_add(this._lw_ref, '' + text);
            return this;
        },
        toString: function()
        {   return exports.lwjs_error_tostring(this._lw_ref);
        }
    };
    
    /*** Webserver ***/
    
    (Lacewing.Webserver = function(eventPump)
    {
        if(!eventPump instanceof Lacewing.EventPump)
            throw "EventPump invalid or not specified";
        
        var self = this, callbacks = {};
        addBinders(this, callbacks, 'error get post head connect disconnect');
        
        this._lw_ref = exports.lwjs_ws_new(eventPump._lw_ref, function(_callback)
        {   try
            {   var ret, callback = '' + _callback,
                    list = callbacks[callback],
                    params = Array.prototype.slice.call(arguments);
                    
                params.shift();
                
                if(callback == 'error')
                    params[0] = new Lacewing.Error(exports, params[0])
                else
                    params[0] = new Lacewing.Webserver.Request(exports, params[0]);
                    
                for(var i = 0; i < list.length; ++ i)
                    ret = list[i].apply(self, params);
                
                return ret;
            }
            catch(e)
            {
            }
        });
        
        return this;
        
    }).prototype =
    {
        host: function(port)
        {
            if(port === undefined)
                port = 80;
                
            if(typeof(port) == 'number')
                return exports.lwjs_ws_host(this._lw_ref, port);
                
            if(port instanceof Lacewing.Filter)
                return exports.lwjs_ws_host_filter(this._lw_ref, port);
                
            throw "Invalid port";
        },
        hostSecure: function(port)
        {
            if(port === 'undefined')
                port = 443;
                
            if(typeof(port) === 'number')
                return exports.lwjs_ws_host_secure(this._lw_ref, port);
                
            return exports.lwjs_ws_host_secure_filter(this._lw_ref, port);
        },
        unhost: function()
        {   exports.lwjs_ws_unhost(this._lw_ref);
            return this;
        },
        unhostSecure: function()
        {   exports.lwjs_ws_unhost(this._lw_ref);
            return this;
        },
        hosting: function()
        {   return exports.lwjs_ws_hosting(this._lw_ref);
        },
        hostingSecure: function()
        {   exports.lwjs_ws_hosting_secure(this._lw_ref);
            return this;
        },
        port: function()
        {   return exports.lwjs_ws_port(this._lw_ref);
        },
        portSecure: function()
        {   return exports.lwjs_ws_port_secure(this._lw_ref);
        },
        loadCertificateFile: function(filename, passphrase)
        {   return exports.lwjs_ws_load_cert_file(this._lw_ref, filename ? filename : '',
                            passphrase ? passphrase : '');
        },
        loadSystemCertificate: function(storeName, commonName, location)
        {   return exports.lwjs_ws_load_sys_cert(this._lw_ref, storeName ? storeName : '',
                    commonName ? commonName : '', location ? location : '');
        },
        certificateLoaded: function()
        {   return exports.lwjs_ws_cert_loaded(this._lw_ref);
        },
        bytesSent: function()
        {   return exports.lwjs_ws_bytes_sent(this._lw_ref);
        },
        bytesReceived: function()
        {   return exports.lwjs_ws_bytes_received(this._lw_ref);
        },
        closeSession: function(session)
        {   exports.lwjs_ws_close_session(this._lw_ref, session);
            return this;
        },
        enableManualRequestFinish: function()
        {   exports.lwjs_ws_enable_manual_finish(this._lw_ref);
            return this;
        }
    };
    
    (Lacewing.Webserver.Request = function(e, ref)
    {
        /* exports is only accessible from within the liblacewing closure, so
           anything in this file will be able to create Request objects by
           passing exports, but anything outside won't. */
           
        if(e !== exports)
            throw "Can't create Webserver.Request objects";
        
        this._lw_ref = ref;
        
    }).prototype =
    {
        write: function(text)
        {   exports.lwjs_ws_req_send_text(this._lw_ref, text);
            return this;
        },
        writeFile: function(filename)
        {   exports.lwjs_ws_req_sendfile(this._lw_ref, filename);
            return this;
        },
        send: function(text)
        {   exports.lwjs_ws_req_send_text(this._lw_ref, text);
            return this;
        },
        sendFile: function(filename)
        {   exports.lwjs_ws_req_sendfile(this._lw_ref, filename);
            return this;
        },
        reset: function()
        {   exports.lwjs_ws_req_reset(this._lw_ref);
            return this;
        },
        finish: function()
        {   exports.lwjs_ws_req_finish(this._lw_ref);
            return this;
        },
        disconnect: function()
        {   exports.lwjs_ws_req_disconnect(this._lw_ref);
            return this;
        },
        redirect: function(url)
        {   exports.lwjs_ws_req_set_redirect(this._lw_ref, url);
            return this;
        },
        responseType: function(code, message)
        {   exports.lwjs_ws_req_set_response_type(this._lw_ref, code, message ? message : '');
            return this;
        },
        mimeType: function(type)
        {   exports.lwjs_ws_req_set_mime_type(this._lw_ref, type);
            return this;
        },
        guessMimeType: function(type)
        {   exports.lwjs_ws_req_set_mime_type(this._lw_ref, type);
            return this;
        },
        charset: function(charset)
        {   exports.lwjs_ws_req_set_charset(this._lw_ref, charset);
            return this;
        },
        setUnmodified: function()
        {   exports.lwjs_ws_req_set_unmodified(this._lw_ref);
            return this;
        },
        get address ()
        {   return new Lacewing.Address(exports, exports.lwjs_ws_req_addr(this._lw_ref));
        },
        get secure ()
        {   return exports.lwjs_ws_req_secure(this._lw_ref);
        },
        get URL ()
        {   return exports.lwjs_ws_req_url(this._lw_ref);
        },
        get hostname ()
        {   return exports.lwjs_ws_req_hostname(this._lw_ref);
        },
        get lastModified ()
        {   return exports.lwjs_ws_req_last_modified(this._lw_ref);
        },
        set lastModified (modified)
        {   exports.lwjs_ws_req_set_last_modified(this._lw_ref, modified);
        },
        header: function(name, value)
        {
            if(value === undefined)
                return exports.lwjs_ws_req_header(this._lw_ref, name);

            exports.lwjs_ws_req_set_header(this._lw_ref, name, value);
            return this;
        },
        cookie: function(name, value)
        {
            if(value === undefined)
                return exports.lwjs_ws_req_cookie(this._lw_ref, name);

            exports.lwjs_ws_req_set_cookie(this._lw_ref, name, value);
            return this;
        },
        session: function(name, value)
        {
            if(value === undefined && name === undefined)
                return exports.lwjs_ws_req_session_id(this._lw_ref);
                
            if(value === undefined)
                return exports.lwjs_ws_req_session_read(this._lw_ref, name);
        
            exports.lwjs_ws_req_session_write(this._lw_ref, name, value);
            return this;
        },
        closeSession: function()
        {   exports.lwjs_ws_req_session_close(this._lw_ref);
            return this;
        },
        GET: function(name)
        {   return exports.lwjs_ws_req_GET(this._lw_ref, name);
        },
        POST: function(name)
        {   return exports.lwjs_ws_req_POST(this._lw_ref, name);
        },
        disableCache: function()
        {   exports.lwjs_ws_req_disable_cache(this._lw_ref);
            return this;
        }
    };
    
    /*** Address ***/
    
    (Lacewing.Address = function(a, b)
    {
       if(a === undefined)
           this._lw_ref = exports.lwjs_address_new();
       else if(typeof(a) == 'string')
           this._lw_ref = (b ? exports.lwjs_address_new_name_blocking
                                : exports.lwjs_address_new_name)(a);
       else if(a instanceof Lacewing.Address)
           this._lw_ref = exports.lwjs_address_copy(a._lw_ref);
       else if(a === exports)
           this._lw_ref = b;
       else throw "Invalid parameters";
           
       return this;

    }).prototype =
    {
        port: function(port)
        {
            if(port !== undefined)
            {   exports.lwjs_address_set_port(this._lw_ref, port);
                return this;
            }
            else
                return exports.lwjs_address_port(this._lw_ref);
        },
        ready: function()
        {   return exports.lwjs_address_is_ready(this._lw_ref);
        },
        toString: function()
        {   return exports.lwjs_address_tostring(this._lw_ref);
        }
    };
    
    return Lacewing;
})



