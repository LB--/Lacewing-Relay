
/* See `send_file.c` for a C version */

#include <lacewing.h>

void on_get (lacewing::webserver, lacewing::webserver_request request)
{
    /* The MIME type defaults to "text/html" */

    request->set_mimetype ("text/plain");
 

    /* Nothing will actually be sent until after the handler returns - every
     * function in liblacewing is non-blocking.
     *
     * This hook will complete instantly, and then liblacewing will continue
     * sending the actual data afterwards.  This is important for large files!
     *
     * Data can be sent between files, and multiple files can be sent in a
     * row, and so on - liblacewing will keep everything in the correct order.
     */
       
    request->writef ("Here's my source:\r\n\r\n");
    request->write_file ("send_file.cc");
}

int main (int argc, char * argv [])
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

