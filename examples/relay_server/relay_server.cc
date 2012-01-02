
#include <Lacewing.h>
#include <stdio.h>

int main (int argc, char * argv [])
{
    Lacewing::EventPump EventPump;
    Lacewing::RelayServer Server (EventPump);

    Server.Host ();

    printf ("Hosting on port %d\n", Server.Port ());

    EventPump.StartEventLoop ();

    return 0;
}

