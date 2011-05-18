
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
    
    /* Data can be written between SendFile calls, and multiple files can be
       sent in a row, etc etc - Lacewing will handle keeping everything in order.
       
       It's also worth noting that nothing will actually be sent until after the
       handler returns - all the methods in the request object are non-blocking.

       The handler will complete instantly, and then Lacewing will take its time
       sending the actual data afterwards.  This is important for large files! */
       
    request.write("Here's my source:\r\n\r\n")
           .sendFile('send_file.js');
});

webserver.host(8080);

