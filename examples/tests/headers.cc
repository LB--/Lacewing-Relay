
#include <Lacewing.h>

void onGet(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
{
    Request << "<p>Hello world from " << Lacewing::Version() << "</p>";
    
    Request << "<p>Headers used in this request:</p><p>";
   
    for (Lacewing::Webserver::Request::Header * Header = Request.FirstHeader ();
                Header; Header = Header->Next ()
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
    Webserver.Host (80);    
    
    EventPump.StartEventLoop();
    
    return 0;
}

