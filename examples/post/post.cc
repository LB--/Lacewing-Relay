
#include <lacewing.h>

void on_get (lacewing::webserver, lacewing::webserver_request request)
{
    request->write_file ("post.html");
}

void on_post (lacewing::webserver, lacewing::webserver_request request)
{
    request->writef ("Hello %s!  You posted:<br /><br />", request->POST ("name"));

    for (lacewing::webserver_request_param param
            = request->POST (); param; param = param->next ())
    {
        request->writef ("%s = %s", param->name (), param->value ());
    }
}

int main (int argc, char * argv [])
{
    lacewing::eventpump eventpump = lacewing::eventpump_new ();
    lacewing::webserver webserver = lacewing::webserver_new (eventpump);

    webserver->on_get (on_get);
    webserver->on_post (on_post);

    webserver->host (8080);

    eventpump->start_eventloop ();

    lacewing::webserver_delete (webserver);
    lacewing::pump_delete (eventpump);

    return 0;
}

