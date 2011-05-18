
/*
    Simple hello world webserver example (C)
   
    - See hello_world.cc for a C++ version
    - See hello_world.js for a Javascript version
*/

#include <Lacewing.h>

void onGet(lw_ws * webserver, lw_ws_req * request)
{
    lw_ws_req_sendf(request, "Hello world");
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

