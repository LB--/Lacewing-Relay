
#include <lacewing.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

size_t plus_one (lw_stream stream, const char * data, size_t size)
{
    char * copy = (char *) malloc (size);

    for (size_t i = 0; i < size; ++ i)
        copy [i] = data [i] + 1;

    lw_stream_data (stream, copy, size);

    free (copy);

    return size;
}

void on_data (lacewing::stream, void * tag, const char * buffer, size_t size)
{
    lw_dump (buffer, size);
}

int main (int argc, char * argv [])
{
    lacewing::stream stream = lacewing::pipe_new (0);

    stream->add_hook_data (on_data, 0);

    printf ("AAA: \n");
    stream->writef ("AAA");

    lw_streamdef plus_one_streamdef = {};
    plus_one_streamdef.sink_data = plus_one;

    stream->add_filter_upstream (lacewing::stream_new (&plus_one_streamdef, 0), true, true);

    lacewing::stream_delete (stream);

    return 0;
}

