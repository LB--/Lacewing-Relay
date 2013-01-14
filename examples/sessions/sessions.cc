
#include <lacewing.h>

void on_get (lacewing::webserver, lacewing::webserver_request request)
{
    request->write_file ("sessions.html");
}

void on_post (lacewing::webserver, lacewing::webserver_request request)
{
    request->writef ("Your session previously stored: %s", request->session ("data"));

    const char * data = request->POST ("data");

    if (*data)
        request->session ("data", data);

    request->writef ("<br />Now it stores: %s", request->session ("data"));
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

