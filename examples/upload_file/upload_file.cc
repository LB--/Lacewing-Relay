
#include <Lacewing.h>

void onGet (Lacewing::Webserver &, Lacewing::Webserver::Request &request)
{
    request.WriteFile ("upload_file.html");
}

void onUploadStart (Lacewing::Webserver &,
                    Lacewing::Webserver::Request &request,
                    Lacewing::Webserver::Upload &upload)
{
    upload.SetAutoSave ();
}

void onUploadPost (Lacewing::Webserver &,
                   Lacewing::Webserver::Request &request,
                   Lacewing::Webserver::Upload * uploads [],
                   int upload_count)
{
    request << "Hello " << request.POST ("name") << "!<br /><br />";

    for (int i = 0; i < upload_count; ++ i)
    {
        request << "You uploaded: <b>" << uploads [i]->Filename () << "</b><br /><pre>";
        request.WriteFile (uploads [i]->GetAutoSaveFilename ());
        request << "</pre><br /><br />";
    }
}

int main (int argc, char * argv [])
{
    Lacewing::EventPump event_pump;
    Lacewing::Webserver webserver (event_pump);

    webserver.onGet (onGet);
    webserver.onUploadStart (onUploadStart);
    webserver.onUploadPost (onUploadPost);

    webserver.Host (8080);

    event_pump.StartEventLoop ();
}

