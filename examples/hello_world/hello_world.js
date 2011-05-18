
/*
    Simple hello world webserver example (Javascript)
    
    - See hello_world.cc for a C++ version
    - See hello_world.c for a C version
*/

Lacewing = require('liblacewing');
webserver = new Lacewing.Webserver();

webserver.bind('get', function(request)
{
    request.write('Hello world from ' + Lacewing.version());
});

webserver.host(8080);


