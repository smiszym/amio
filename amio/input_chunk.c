#include "input_chunk.h"

#include <stdlib.h>
#include <string.h>

struct InputChunk input_chunk_being_read;

int InputChunk_get_playspec_id()
{
    /* Runs on the Python thread */

    return input_chunk_being_read.playspec_id;
}

int InputChunk_get_starting_frame()
{
    /* Runs on the Python thread */

    return input_chunk_being_read.starting_frame;
}

int InputChunk_get_was_transport_rolling()
{
    /* Runs on the Python thread */

    return input_chunk_being_read.was_transport_rolling;
}

int InputChunk_get_samples(char *bytearray, int n)
{
    /* Runs on the Python thread */

    if (n != INPUT_CLIP_LENGTH * sizeof(jack_default_audio_sample_t))
        return 0;

    memcpy(bytearray, input_chunk_being_read.samples, n);
    return 1;
}
