
#include <lacewing.h>

void on_get (lw_ws ws, lw_ws_req request)
{
    lw_stream_write_file (request, "upload_file.html");
}

void on_upload_start (lw_ws ws,
                      lw_ws_req request,
                      lw_ws_upload upload)
{
    lw_ws_upload_set_autosave (upload);
}

void on_upload_post (lw_ws ws,
                     lw_ws_req request,
                     lw_ws_upload uploads [],
                     size_t num_uploads)
{
    size_t i;

    lw_stream_writef (request, "Hello %s!<br /><br />", lw_ws_req_POST (request, "name"));

    for (i = 0; i < num_uploads; ++ i)
    {
        lw_stream_writef (request, "You uploaded: <b>%s</b><br /><pre>", lw_ws_upload_filename (uploads [i]));
        lw_stream_write_file (request, lw_ws_upload_autosave_fname (uploads [i]));
        lw_stream_writef (request, "</pre><br /><br />");
    }
}

int main (int argc, char * argv [])
{
    lw_eventpump eventpump;
    lw_ws webserver;
  
    eventpump = lw_eventpump_new ();
    webserver = lw_ws_new (eventpump);

    lw_ws_on_get (webserver, on_get);
    lw_ws_on_upload_start (webserver, on_upload_start);
    lw_ws_on_upload_post (webserver, on_upload_post);

    lw_ws_host (webserver, 8080);

    lw_eventpump_start_eventloop (eventpump);

    lw_ws_delete (webserver);
    lw_pump_delete (eventpump);

    return 0;
}

