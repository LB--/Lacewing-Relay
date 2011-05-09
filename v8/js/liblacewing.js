
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
    };

    Lacewing.version = function()
    {   return exports.lwjs_version();
    };
    
    /*** EventPump ***/
    
    (Lacewing.EventPump = function()
    {            
        var ref = exports.lwjs_eventpump_new();
        
        this.ref = function()
        {   return ref;
        }
    
    }).prototype =
    {
        tick: function()
        {   exports.lwjs_eventpump_tick(this.ref());
        },
        
        startEventLoop: function()
        {   exports.lwjs_eventpump_start_event_loop(this.ref());
        },
    };
    
    
    /*** Webserver ***/
    
    (Lacewing.Webserver = function(eventPump)
    {
        if(!eventPump instanceof Lacewing.EventPump)
            throw "EventPump invalid or not specified";
        
        var self = this, callbacks = {};
        
        this.bind = function(ev, fn)
        {
            if(!callbacks[ev])
                callbacks[ev] = [ fn ];
            else
                callbacks[ev].push(fn);
        }
        
        var ref = exports.lwjs_ws_new(eventPump.ref(), function(_callback)
        {
            try
            {
                var callback = '' + _callback;
                var list = callbacks[callback];
                
                if(!list)
                    return;
                    
                var params = Array.prototype.slice.call(arguments), ret;
                params.shift();
                
                if(callback != 'error')
                    params[0] = new Lacewing.Webserver.Request(exports, params[0]);
                    
                for(var i = 0; i < list.length; ++ i)
                    ret = list[i].apply(self, params);
                
                return ret;
            }
            catch(e)
            {
            }
        });
        
        this.ref = function()
        {   return ref;
        }
        
    }).prototype =
    { 
        host: function(port)
        {
            if(port === undefined)
                port = 80;
                
            if(typeof(port) == 'number')
                return exports.lwjs_ws_host(this.ref(), port);
                
            if(port instanceof Lacewing.Filter)
                return exports.lwjs_ws_host_filter(this.ref(), port);
                
            throw "Invalid port";
        },
        hostSecure: function(port)
        {
            if(port === 'undefined')
                port = 443;
                
            if(typeof(port) === 'number')
                return exports.lwjs_ws_host_secure(this.ref(), port);
                
            return exports.lwjs_ws_host_secure_filter(this.ref(), port);
        },
        unhost: function()
        {   exports.lwjs_ws_unhost(this.ref());
        },
        unhostSecure: function()
        {   exports.lwjs_ws_unhost(this.ref());
        },
        hosting: function()
        {   return exports.lwjs_ws_hosting(this.ref());
        },
        hostingSecure: function()
        {   return exports.lwjs_ws_hosting_secure(this.ref());
        },
        port: function()
        {   return exports.lwjs_ws_port(this.ref());
        },
        portSecure: function()
        {   return exports.lwjs_ws_port_secure(this.ref());
        }
    };
    
    (Lacewing.Webserver.Request = function(e, ref)
    {
        /* exports is only accessible from within the liblacewing closure, so
           anything in this file will be able to create Request objects by
           passing exports, but anything outside won't. */
           
        if(e !== exports)
            throw "Can't create Webserver.Request objects";
        
        this.ref = function()
        {   return ref;
        }
        
    }).prototype =
    {
        write: function(text)
        {   exports.lwjs_ws_req_send_text(this.ref(), text);
        }
    };
    
    return Lacewing;
})



