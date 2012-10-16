
#include <lacewing.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

Lacewing::EventPump EventPump;

struct PlusOneFilter : public Lacewing::Stream
{
    size_t Put (const char * data, size_t size)
    {
        char * copy = (char *) malloc (size);
        
        for (size_t i = 0; i < size; ++ i)
            copy [i] = data [i] + 1;
        
        Data (copy, size);

        return size;
    }
};

void onData (Lacewing::Stream &, void * tag, char * buffer, size_t size)
{
    assert (tag == (void *) 0xC0FFEE);

    lw_dump (buffer, size);
}

int main (int argc, char * argv [])
{
    Lacewing::Pipe stream;

    stream.AddHandlerData (onData, (void *) 0xC0FFEE);

    printf ("AAA: \n");
    stream.Write ("AAA");

    stream.AddFilterUpstream (*new PlusOneFilter, true, true);

    printf ("BBB: \n");
    stream.Write ("AAA");

    stream.AddFilterDownstream (*new PlusOneFilter, true, true);

    printf ("CCC: \n");
    stream.Write ("AAA");

    stream.AddFilterUpstream (*new PlusOneFilter, true, true);

    printf ("DDD: \n");
    stream.Write ("AAA");

    stream.AddFilterDownstream (*new PlusOneFilter, true, true);

    printf ("EEE: \n");
    stream.Write ("AAA");

    {   PlusOneFilter stream2;
        stream.Write (stream2);

        printf ("FFF: \n");
        stream2.Write ("AAA");

        stream2.AddFilterDownstream (*new PlusOneFilter, true, true);

        printf ("GGG: \n");
        stream2.Write ("AAA");

        stream2.AddFilterUpstream (*new PlusOneFilter, true, true);

        printf ("HHH: \n");
        stream2.Write ("AAA");
    }

    printf ("EEE: \n");
    stream.Write ("AAA");

    return 0;
}

