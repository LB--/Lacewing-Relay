
/* vim: set et ts=4 sw=4 ft=javascript:
 *
 * Copyright (C) 2011 James McLaughlin.  All rights reserved.
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

(function(exports, Lacewing)
{
    var addBinders = function(c, callbacks, on)
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
    
    }, getEventPumpRef = function(eventPump)
    {
        if(exports.lwjs_global_eventpump)
            return exports.lwjs_global_eventpump;
            
        if(!(eventPump instanceof Lacewing.EventPump))
            throw "EventPump invalid or not specified";
            
        return eventPump._lw_ref;
    };
      
    /*** Global ***/
    
    Lacewing.version = function()
    {  return exports.lwjs_version();
    };
    Lacewing.lastModified = function(filename)
    {  return exports.lwjs_file_last_modified('' + filename);
    };
    Lacewing.fileExists = function(filename)
    {  return exports.lwjs_file_exists('' + filename);
    };
    Lacewing.fileSize = function(filename)
    {  return exports.lwjs_file_size('' + filename);
    };
    Lacewing.pathExists = function(filename)
    {  return exports.lwjs_path_exists('' + filename);
    };
    Lacewing.tempPath = function()
    {  return exports.lwjs_temp_path();
    };
    Lacewing.newTempFile = function()
    {  return exports.lwjs_new_temp_file();
    };
    Lacewing.guessMimeType = function(filename)
    {  return exports.lwjs_guess_mime_type('' + filename);
    };
    Lacewing.md5 = function(str)
    {  return exports.lwjs_md5('' + str);
    };
    Lacewing.sha1 = function(str)
    {  return exports.lwjs_sha1('' + str);
    };
    
    /*** EventPump ***/
    
    if(exports.lwjs_eventpump_new)
    {
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
    }
    
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
        var eventPumpRef = getEventPumpRef(eventPump), self = this, callbacks = {};
        addBinders(this, callbacks, 'error get post head connect disconnect');
        
        this._lw_ref = exports.lwjs_ws_new(eventPumpRef, function(_callback)
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
                return exports.lwjs_ws_host_filter(this._lw_ref, port._lw_ref);
                
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
        {   exports.lwjs_ws_close_session(this._lw_ref, '' + session);
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
        {   exports.lwjs_ws_req_send_text(this._lw_ref, '' + text);
            return this;
        },
        writeFile: function(filename)
        {   exports.lwjs_ws_req_sendfile(this._lw_ref, '' + filename);
            return this;
        },
        send: function(text)
        {   exports.lwjs_ws_req_send_text(this._lw_ref, '' + text);
            return this;
        },
        sendFile: function(filename)
        {   exports.lwjs_ws_req_sendfile(this._lw_ref, '' + filename);
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
        {   exports.lwjs_ws_req_set_redirect(this._lw_ref, '' + url);
            return this;
        },
        responseType: function(code, message)
        {   exports.lwjs_ws_req_set_response_type(this._lw_ref, 1 * code, '' + (message ? message : ''));
            return this;
        },
        mimeType: function(type, charset)
        {
            if (charset)
                exports.lwjs_ws_req_set_mime_type_ex(this._lw_ref, '' + type, '' + charset);
            else
                exports.lwjs_ws_req_set_mime_type(this._lw_ref, '' + type);
                
            return this;
        },
        guessMimeType: function(type)
        {   exports.lwjs_ws_req_set_mime_type(this._lw_ref, '' + type);
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
        {   exports.lwjs_ws_req_set_last_modified(this._lw_ref, 1 * modified);
        },
        header: function(name, value)
        {
            if(value === undefined)
                return exports.lwjs_ws_req_header(this._lw_ref, '' + name);

            exports.lwjs_ws_req_set_header(this._lw_ref, '' + name, '' + value);
            return this;
        },
        cookie: function(name, value)
        {
            if(value === undefined)
                return exports.lwjs_ws_req_cookie(this._lw_ref, '' + name);

            exports.lwjs_ws_req_set_cookie(this._lw_ref, '' + name, '' + value);
            return this;
        },
        session: function(name, value)
        {
            if(value === undefined && name === undefined)
                return exports.lwjs_ws_req_session_id(this._lw_ref);
                
            if(value === undefined)
                return exports.lwjs_ws_req_session_read(this._lw_ref, '' + name);
        
            exports.lwjs_ws_req_session_write(this._lw_ref, '' + name, '' + value);
            return this;
        },
        closeSession: function()
        {   exports.lwjs_ws_req_session_close(this._lw_ref);
            return this;
        },
        GET: function(name)
        {   return exports.lwjs_ws_req_GET(this._lw_ref, '' + name);
        },
        POST: function(name)
        {   return exports.lwjs_ws_req_POST(this._lw_ref, '' + name);
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
                                : exports.lwjs_address_new_name)('' + a);
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
            {   exports.lwjs_address_set_port(this._lw_ref, 1 * port);
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
    
    /*** Filter ***/
    
    (Lacewing.Filter = function(a, b)
    {
       if(a === undefined)
           this._lw_ref = exports.lwjs_filter_new();
       else if(a === exports)
           this._lw_ref = b;
       else throw "Invalid parameters";
           
       return this;

    }).prototype =
    {
        local: function(a)
        {
            if(a === undefined)
                return exports.lwjs_filter_get_local_ip(this._lw_ref);
                
            if(typeof(a) === 'string')
            {   exports.lwjs_filter_set_local(this._lw_ref, a);
                return this;
            }
            
            throw "Invalid arguments";
            return this;
        },
        remote: function(a)
        {
            if(a === undefined)
                return new Lacewing.Address(exports, exports.lwjs_filter_get_remote_addr(this._lw_ref));
                
            if(a instanceof Lacewing.Address)
            {   exports.lwjs_filter_set_remote_addr(this._lw_ref, a.lw_ref);
                return this;
            }
            
            if(typeof(a) === 'string')
            {   exports.lwjs_filter_set_remote(this._lw_ref, a);
                return this;
            }
            
            throw "Invalid arguments";
            return this;
        },
        localPort: function(port)
        {
            if(port === undefined)
                return exports.lwjs_filter_get_local_port(this._lw_ref);
                
            exports.lwjs_filter_set_local_port(this._lw_ref, port);
            return this;
        },
        reuse: function(enabled)
        {
            if(enabled === undefined)
                return exports.lwjs_filter_is_reuse_set(this._lw_ref);
         
            if(enabled === true || enabled === false)
                exports.lwjs_filter_set_reuse(enabled);
                
            return this;
        }
    };
    
    return Lacewing;
})



