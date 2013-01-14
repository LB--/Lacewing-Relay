
#include <lacewing.h>

void on_get (lacewing::webserver, lacewing::webserver_request request)
{
    request->write_file ("upload_file.html");
}

void on_upload_start (lacewing::webserver,
                      lacewing::webserver_request request,
                      lacewing::webserver_upload upload)
{
    upload->set_autosave ();
}

void on_upload_post (lacewing::webserver,
                     lacewing::webserver_request request,
                     lacewing::webserver_upload uploads [],
                     size_t num_uploads)
{
    request->writef ("Hello %s!<br /><br />", request->POST ("name"));

    for (size_t i = 0; i < num_uploads; ++ i)
    {
        request->writef ("You uploaded: <b>%s</b><br /><pre>", uploads [i]->filename ());
        request->write_file (uploads [i]->autosave_filename ());
        request->writef ("</pre><br /><br />");
    }
}

int main (int argc, char * argv [])
{
    lacewing::eventpump eventpump = lacewing::eventpump_new ();
    lacewing::webserver webserver = lacewing::webserver_new (eventpump);

    webserver->on_get (on_get);
    webserver->on_upload_start (on_upload_start);
    webserver->on_upload_post (on_upload_post);

    webserver->host (8080);

    eventpump->start_eventloop ();

    lacewing::webserver_delete (webserver);
    lacewing::pump_delete (eventpump);

    return 0;
}

