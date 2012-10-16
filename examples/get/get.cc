
#include <lacewing.h>
#include <string.h>

void onGet (Lacewing::Webserver &, Lacewing::Webserver::Request &request)
{
    if (!strcmp (request.URL (), ""))
    {
        request.WriteFile ("get.html");
        return;
    }

    if (!strcmp (request.URL (), "get"))
    {
        request << "Hello " << request.GET ("name") << "!  GET parameters:<br /><br />";

        for (Lacewing::Webserver::Request::Parameter * p
                = request.GET (); p; p = p->Next ())
        {
            request << p->Name () << " = " << p->Value () << "<br />";
        }
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

