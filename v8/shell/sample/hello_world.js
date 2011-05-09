
/* Load me in the shell : load('sample/hello_world.js') */

print("Hosting hello world example...");

eventPump = new Lacewing.EventPump();
webserver = new Lacewing.Webserver(eventPump);

webserver.bind('get', function(request)
{   request.write('Hello world from JS!');
});

webserver.host(80);
eventPump.startEventLoop();

