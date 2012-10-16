
#include <lacewing.h>

void onGet (Lacewing::Webserver &, Lacewing::Webserver::Request &request)
{
    request.WriteFile ("post.html");
}

void onPost (Lacewing::Webserver &, Lacewing::Webserver::Request &request)
{
    request << "Hello " << request.POST ("name") << "!  You posted:<br /><br />";

    for (Lacewing::Webserver::Request::Parameter * p
            = request.POST (); p; p = p->Next ())
    {
        request << p->Name () << " = " << p->Value () << "<br />";
    }
}

int main (int argc, char * argv [])
{
    Lacewing::EventPump event_pump;
    Lacewing::Webserver webserver (event_pump);

    webserver.onGet (onGet);
    webserver.onPost (onPost);

    webserver.Host (8080);

    event_pump.StartEventLoop ();
}

