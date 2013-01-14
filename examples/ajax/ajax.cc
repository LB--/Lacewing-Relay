
#include <lacewing.h>

#include <string.h>
#include <iostream>
#include <list>

using namespace std;

list <lacewing::webserver_request> waiting;

void on_get (lacewing::webserver, lacewing::webserver_request request)
{
    request->write_file ("ajax.html");
    request->finish ();        
}

void on_post (lacewing::webserver, lacewing::webserver_request request)
{
    if (!strcmp (request->url (), "poll"))
    {
        waiting.push_back (request);
        return;
    }
    
    if (!strcmp (request->url (), "message"))
    {
        const char * message = request->POST ("message");
        
        request->writef ("Message from %s: %s\n",
                         request->address ()->tostring (),
                         message);

        /* Complete all waiting requests with the message */
        
        for(list <lacewing::webserver_request>::iterator it
                = waiting.begin (); it != waiting.end (); ++ it)
        {
            lacewing::webserver_request waiting_req = *it;
            
            waiting_req->write (message);
            waiting_req->finish ();
        }

        waiting.clear ();
        request->finish ();
        
        return;
    }
}

void on_disconnect (lacewing::webserver, lacewing::webserver_request request)
{
    for (list <lacewing::webserver_request>::iterator it
            = waiting.begin(); it != waiting.end (); ++ it)
    {
        if(*it == request)
        {
            waiting.erase (it);
            break;
        }
    }
}

int main (int argc, char * argv[])
{
    lacewing::eventpump eventpump = lacewing::eventpump_new ();
    lacewing::webserver webserver = lacewing::webserver_new (eventpump);

    /* Enabling this means we will have to call request->finish() to complete
     * a request.  Until request->finish() is called, requests will just hang,
     * which is exactly what we want for long-poll AJAX.
     */
       
    webserver->enable_manual_finish ();
    
    webserver->on_get (on_get);
    webserver->on_post (on_post);
    webserver->on_disconnect (on_disconnect);
   
    webserver->host (8080);    
    
    eventpump->start_eventloop();
    
    lacewing::webserver_delete (webserver);
    lacewing::pump_delete (eventpump);

    return 0;
}

