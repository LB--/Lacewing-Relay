
/* SendFile example (C++).  See send_file.c for a C version */

#include <Lacewing.h>

void onGet(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
{
    Request.SetMimeType("text/plain");
 
    /* Data can be sent between SendFile calls, and multiple files can be sent in
       a row, etc etc - Lacewing will handle keeping everything in order.
       
       It's also worth noting that nothing will actually be sent until after the
       handler returns - all the methods in the Request class are non-blocking.

       The handler will complete instantly, and then Lacewing will take its time
       sending the actual data afterwards.  This is important for large files! */
       
    Request << "Here's my source:\r\n\r\n";
    Request.SendFile("send_file.cc");
}

int main(int argc, char * argv[])
{
    Lacewing::EventPump EventPump;
    Lacewing::Webserver Webserver(EventPump);

    Webserver.onGet(onGet);
    Webserver.Host(80);    
    
    EventPump.StartEventLoop();
    
    return 0;
}

