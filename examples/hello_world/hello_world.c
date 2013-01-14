
/* See `hello_world.cc` for a C++ version */

#include <lacewing.h>

void on_get (lw_ws webserver, lw_ws_req req)
{
   lw_stream_writef (req, "Hello world from %s", lw_version ());
}

int main (int argc, char * argv[])
{
   lw_pump pump = lw_eventpump_new ();
   lw_ws webserver = lw_ws_new (pump);

   lw_ws_on_get (webserver, on_get);
   lw_ws_host (webserver, 8081);
    
   lw_eventpump_start_eventloop (pump);
    
   lw_ws_delete (webserver);
   lw_pump_delete (pump);

   return 0;
}

