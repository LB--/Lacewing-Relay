
/* See `send_file.cc` for a C++ version */

#include <lacewing.h>

void onGet(lw_ws * webserver, lw_ws_req * request)
{
    /* The MIME type defaults to "text/html" */

    lw_ws_req_set_mime_type(request, "text/plain");

    
    /* Nothing will actually be sent until after the handler returns - all the
     * methods in the Request class are non-blocking.
     *
     * The handler will complete instantly, and then liblacewing will continue
     * sending the actual data afterwards.  This is important for large files!
     *
     * Data can be sent between files, and multiple files can be sent in a
     * row, etc etc - liblacewing will keep everything in order.
     */
       
    lw_stream_writef(request, "Here's my source:\r\n\r\n");
    lw_stream_write_file(request, "send_file.c");
}

int main(int argc, char * argv[])
{
    lw_eventpump * eventpump = lw_eventpump_new();
    lw_ws * webserver = lw_ws_new(eventpump);

    lw_ws_onget(webserver, onGet);
    lw_ws_host(webserver, 8080);
    
    lw_eventpump_start_event_loop(eventpump);
    
    return 0;
}

