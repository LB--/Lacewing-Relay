
#include <lacewing.h>

#include <string.h>
#include <iostream>
#include <list>

using namespace std;

list <Lacewing::Webserver::Request *> waiting;

void onGet(Lacewing::Webserver &, Lacewing::Webserver::Request &Request)
{
    Request.WriteFile("ajax.html");
    Request.Finish();        
}

void onPost(Lacewing::Webserver &, Lacewing::Webserver::Request &request)
{
    if(!strcmp(request.URL(), "poll"))
    {
        waiting.push_back(&request);
        return;
    }
    
    if(!strcmp(request.URL(), "message"))
    {
        const char * message = request.POST("message");
        
        cout << "Message from " << request.GetAddress() << ": " << message << endl;

        /* Complete all waiting requests with the message */
        
        for(list<Lacewing::Webserver::Request *>::iterator it
                = waiting.begin(); it != waiting.end(); ++ it)
        {
            Lacewing::Webserver::Request &waiting_req = **it;
            
            waiting_req << message;
            waiting_req.Finish();
        }

        waiting.clear();        
        request.Finish();
        
        return;
    }
}

void onDisconnect(Lacewing::Webserver &, Lacewing::Webserver::Request &Request)
{
    for(list<Lacewing::Webserver::Request *>::iterator it
            = waiting.begin(); it != waiting.end(); ++ it)
    {
        if(*it == &Request)
        {
            waiting.erase(it);
            break;
        }
    }
}

int main(int argc, char * argv[])
{
    Lacewing::EventPump EventPump;
    Lacewing::Webserver Webserver(EventPump);

    /* Enabling this means we will have to call Request.Finish() to complete
     * a request.  Until Request.Finish() is called, requests will just hang,
     * which is exactly what we want for long-poll AJAX.
     */
       
    Webserver.EnableManualRequestFinish();
    
    Webserver.onGet(onGet);
    Webserver.onPost(onPost);
    Webserver.onDisconnect(onDisconnect);
   
    Webserver.Host(8080);    
    
    EventPump.StartEventLoop();
    
    return 0;
}

