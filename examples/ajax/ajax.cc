
/* AJAX example (C++) - this would be very possible in C and Javascript, too,
   but I haven't translated the example. */

#include <Lacewing.h>

#include <iostream>
#include <list>
using namespace std;

list <Lacewing::Webserver::Request *> WaitingRequests;

void onGet(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
{
    Request.SendFile("ajax.html");
    Request.Finish();        
}

void onPost(Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request)
{
    if(!strcmp(Request.URL(), "poll"))
    {
        WaitingRequests.push_back(&Request);
        return;
    }
    
    if(!strcmp(Request.URL(), "message"))
    {
        const char * Message = Request.POST("message");
        
        /* Loop through all the waiting requests, sending them the message and
           finishing them. */
        
        for(list<Lacewing::Webserver::Request *>::iterator it = WaitingRequests.begin();
                it != WaitingRequests.end(); ++ it)
        {
            Lacewing::Webserver::Request &WaitingRequest = **it;
            
            WaitingRequest << Message;
            WaitingRequest.Finish();
        }

        WaitingRequests.clear();        
        Request.Finish();
        
        return;
    }
}

int main(int argc, char * argv[])
{
    Lacewing::EventPump EventPump;
    Lacewing::Webserver Webserver(EventPump);

    /* Enabling this means we will have to call Request.Finish() to complete
       a request.  Until we do this, requests will just "hang", which is exactly
       how long-poll AJAX works. */
       
    Webserver.EnableManualRequestFinish();
    
    Webserver.onGet(onGet);
    Webserver.onPost(onPost);
    
    Webserver.Host(8080);    
    
    EventPump.StartEventLoop();
    
    return 0;
}

