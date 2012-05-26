
/*
    Simple hello world webserver example (C++)
   
    - See hello_world.c for a C version
    - See hello_world.js for a Javascript version
*/


#include <Lacewing.h>

void onGet(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
{
    Request << "Hello world from " << Lacewing::Version();
}

int main(int argc, char * argv[])
{
    Lacewing::EventPump EventPump;
    Lacewing::Webserver Webserver(EventPump);

    Webserver.onGet(onGet);

    Lacewing::Filter Filter;
    Filter.LocalPort (8080);
    Filter.Reuse (true);

    Webserver.Host(Filter);    
    
    EventPump.StartEventLoop();
    
    return 0;
}

