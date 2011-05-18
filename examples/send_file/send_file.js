
/*
    SendFile example (Javascript)
   
    - See send_file.cc for a C++ version
    - See send_file.c for a C version
*/

Lacewing = require('liblacewing');
webserver = new Lacewing.Webserver();

webserver.bind('get', function(request)
{
    request.mimeType('text/plain');
    
    request.write("Here's my source:\r\n\r\n")
           .sendFile('send_file.js');
});

webserver.host(8080);

