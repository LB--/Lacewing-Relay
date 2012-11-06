
#include <lacewing.h>
#include <stdlib.h>

void onGet (Lacewing::Webserver &webserver, Lacewing::Webserver::Request &request)
{
    request.SetMimeType ("text/plain");

    request << "Here's my source:\r\n\r\n";
    request.WriteFile ("clear_queue.cc");

    if ( (rand () % 2) == 0)
    {
        request.ClearQueue ();

        request << "Changed my mind!  Bwahaha!";
    }
}

int main (int argc, char * argv [])
{
    Lacewing::EventPump event_pump;
    Lacewing::Webserver webserver (event_pump);

    webserver.onGet (onGet);

    webserver.Host (8080);    
    
    event_pump.StartEventLoop ();
}

