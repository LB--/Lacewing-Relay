
/* Simple hello world webserver example (C++)
   See hello_world.c for a C version */

#include <Lacewing.h>

void onGet(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
{
    Request << "Hello world!";
}

int main(int argc, char * argv[])
{
    Lacewing::EventPump EventPump;
    Lacewing::Webserver Webserver(EventPump);

    Webserver.onGet(onGet);
    Webserver.Host(80);    
    
    EventPump.StartEventLoop();
    
    return 0;
}

