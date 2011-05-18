
/** Load me in the shell : load('sample/hello_world.js') **/


print('Hosting hello world example...');

eventPump = new Lacewing.EventPump();
webserver = new Lacewing.Webserver(eventPump);

webserver.bind('get', function(request)
{
    print('Got request from ' + request.address + ' for "' + request.URL + '"');
    
    request.write ('Hello world from JS! <br />')
           .write ('I am ' + Lacewing.version());
});

webserver.bind('error', function(e)
{
    print('Error: ' + e);
});

webserver.host(80);
eventPump.startEventLoop();

