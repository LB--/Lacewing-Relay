
/* AJAX example (Javascript) - see ajax.cc for a C++ version */

waitingRequests = [];

Lacewing = require('liblacewing');
webserver = new Lacewing.Webserver();

/* Enabling this means we will have to call req.finish() to complete
   a request.  Until we do this, requests will just "hang", which is exactly
   how long-poll AJAX works. */
   
webserver.enableManualRequestFinish();

webserver.bind('get', function(req)
{
    req.sendFile('ajax.html').finish();
});

webserver.bind('disconnect', function(req)
{
    var index = waitingRequests.indexOf(req);
    
    if(index != -1)
        waitingRequests.splice(index, 1);
});

webserver.bind('post', function(req)
{
    if(req.URL == 'poll')
    {
        waitingRequests.push(req);
        return;
    }
    
    if(req.URL == 'message')
    {
        var message = req.POST('message');
        
        /* Loop through all the waiting requests, sending them the message and
           finishing them. */
           
        for(var i = 0; i < waitingRequests.length; ++ i)
            waitingRequests[i].write(message).finish();
        
        waitingRequests.length = 0;
        
        req.finish();
    }
});

webserver.host(8080);

