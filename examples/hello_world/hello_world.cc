
/* See `hello_world.c` for a C version */

#include <lacewing.h>

void on_get (lacewing::webserver webserver, lacewing::webserver_request request)
{
    request->writef ("Hello world from %s", lw_version ());
}

int main (int argc, char * argv[])
{
    lacewing::eventpump eventpump = lacewing::eventpump_new ();
    lacewing::webserver webserver = lacewing::webserver_new (eventpump);

    webserver->on_get (on_get);

    webserver->host (8080);    
    
    eventpump->start_eventloop();

    lacewing::webserver_delete (webserver);
    lacewing::pump_delete (eventpump);
    
    return 0;
}

