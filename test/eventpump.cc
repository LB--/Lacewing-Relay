
#include <lacewing.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
    inline void sleep (int n) { Sleep (n * 1000); }
#endif

Lacewing::EventPump EventPump;

void Payload ()
{
    printf ("Payload executed from thread %lld\n", Lacewing::CurrentThreadID ());    
}

void Poster ()
{
    printf ("Poster thread is %lld\n", Lacewing::CurrentThreadID ());

    for (;;)
    {
        EventPump.Post ((void *) Payload);
        sleep (2);
    }
}

void onTick (Lacewing::Timer &Timer)
{
    printf ("Timer ticked from thread %lld\n", Lacewing::CurrentThreadID ());
}

int main (int argc, char * argv [])
{
    printf ("Main thread is %lld\n", Lacewing::CurrentThreadID ());

    Lacewing::Thread Poster ("Poster", (void *) ::Poster);
    Poster.Start ();

    Lacewing::Timer Timer (EventPump);

    Timer.onTick (onTick);
    Timer.Start (3000);

    EventPump.StartEventLoop ();

    return 0;
}

