
#include <lacewing.h>
#include <string.h>

void on_get (lacewing::webserver, lacewing::webserver_request request)
{
    if (!strcmp (request->url (), ""))
    {
        request->write_file ("get.html");
        return;
    }

    if (!strcmp (request->url (), "get"))
    {
        request->writef ("Hello %s!  GET parameters:<br /><br />", request->GET ("name"));

        for (lacewing::webserver_request_param p
                = request->GET (); p; p = p->next ())
        {
            request->writef ("%s = %s<br />", p->name (), p->value ());
        }
    }
}

int main (int argc, char * argv [])
{
    lacewing::eventpump eventpump = lacewing::eventpump_new ();
    lacewing::webserver webserver = lacewing::webserver_new (eventpump);

    webserver->on_get (on_get);

    webserver->host (8080);

    eventpump->start_eventloop ();

    lacewing::webserver_delete (webserver);
    lacewing::pump_delete (eventpump);

    return 0;
}

