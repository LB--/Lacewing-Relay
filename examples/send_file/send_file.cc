
/* See `send_file.c` for a C version */

#include <Lacewing.h>

Lacewing::EventPump EventPump;

void onGet (Lacewing::Webserver &, Lacewing::Webserver::Request &Request)
{
    /* The MIME type defaults to "text/html" */

    Request.SetMimeType ("text/plain");
 

    /* Nothing will actually be sent until after the handler returns - all the
     * methods in the Request class are non-blocking.
     *
     * The handler will complete instantly, and then liblacewing will continue
     * sending the actual data afterwards.  This is important for large files!
     *
     * Data can be sent between files, and multiple files can be sent in a
     * row, etc etc - liblacewing will keep everything in order.
     */
       
    Request << "Here's my source:\r\n\r\n";
    Request.WriteFile ("send_file.cc");
}

int main (int argc, char * argv [])
{
    Lacewing::Webserver Webserver (EventPump);

    Webserver.onGet (onGet);

    Webserver.Host (8080);
    
    EventPump.StartEventLoop();
    
    return 0;
}

