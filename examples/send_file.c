
/* SendFile example (C).  See send_file.cc for a C++ version */

#include <Lacewing.h>

void onGet(lw_ws * webserver, lw_ws_req * request)
{
    lw_ws_req_set_mime_type(request, "text/plain");
    
    /* Data can be sent between sendfile calls, and multiple files can be sent in
       a row, etc etc - Lacewing will handle keeping everything in order.
       
       It's also worth noting that nothing will actually be sent until after the
       handler returns - all the lw_ws_req_* methods are non-blocking.

       The handler will complete instantly, and then Lacewing will take its time
       sending the actual data afterwards.  This is important for large files! */
       
    lw_ws_req_sendf(request, "Here's my source:\r\n\r\n");
    lw_ws_req_sendfile(request, "send_file.c");
}

int main(int argc, char * argv[])
{
    lw_eventpump * eventpump = lw_eventpump_new();
    lw_ws * webserver = lw_ws_new(eventpump);

    lw_ws_onget(webserver, onGet);
    lw_ws_host(webserver, 80);
    
    lw_eventpump_start_event_loop(eventpump);
    
    return 0;
}

