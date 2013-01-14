
/* See `send_file.cc` for a C++ version */

#include <lacewing.h>

void on_get (lw_ws webserver, lw_ws_req request)
{
    /* The MIME type defaults to "text/html" */

    lw_ws_req_set_mimetype (request, "text/plain");

    
    /* Nothing will actually be sent until after the handler returns - every
     * function in liblacewing is non-blocking.
     *
     * This hook will complete instantly, and then liblacewing will continue
     * sending the actual data afterwards.  This is important for large files!
     *
     * Data can be sent between files, and multiple files can be sent in a
     * row, and so on - liblacewing will keep everything in the correct order.
     */
       
    lw_stream_writef (request, "Here's my source:\r\n\r\n");
    lw_stream_write_file (request, "send_file.c");
}

int main(int argc, char * argv[])
{
    lw_pump eventpump = lw_eventpump_new ();
    lw_ws webserver = lw_ws_new (eventpump);

    lw_ws_on_get (webserver, on_get);
    lw_ws_host (webserver, 8080);
    
    lw_eventpump_start_eventloop (eventpump);
    
    lw_ws_delete (webserver);
    lw_pump_delete (eventpump);
    
    return 0;
}

