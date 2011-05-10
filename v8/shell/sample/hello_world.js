
/** Load me in the shell : load('sample/hello_world.js') **/


print('Hosting hello world example...');

eventPump = new Lacewing.EventPump();
webserver = new Lacewing.Webserver(eventPump);

/* Handler for GET requests */
webserver.get(function(request)
{
    print('Got request from ' + request.address + ' for "' + request.URL + '"');
    
    request.write ('Hello world from JS! <br />')
           .write ('I am ' + Lacewing.version());
});

/* Handler for errors */
webserver.error(function(e)
{
    print('Error: ' + e);
});

webserver.host(80);
eventPump.startEventLoop();

