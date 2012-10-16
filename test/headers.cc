
#include <lacewing.h>

#include <assert.h>
#include <string.h>

void onGet(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
{
    Request << "<p>Hello world from " << lw_version () << "</p>";
    Request << "<p>Your User-Agent is: " << Request.Header ("User-Agent") << "</p>";
    
    Request << "<p>Headers used in this request:</p><p>";
   
    for (struct Lacewing::Webserver::Request::Header * Header
                = Request.FirstHeader (); Header; Header = Header->Next ())
    {
        Request << Header->Name () << ": " << Header->Value () << "<br />";
    }
    
    Request << "</p>";
}

int main(int argc, char * argv[])
{
    Lacewing::EventPump EventPump;
    Lacewing::Webserver Webserver(EventPump);

    Webserver.onGet (onGet);
    Webserver.Host (8080);
    
    EventPump.StartEventLoop();
    
    return 0;
}

