#include "input_chunk.h"

#include <stdlib.h>
#include <string.h>

int InputChunk_get_playspec_id(struct InputChunk *clip)
{
    /* Runs on the Python thread */

    return clip->playspec_id;
}

int InputChunk_get_starting_frame(struct InputChunk *clip)
{
    /* Runs on the Python thread */

    return clip->starting_frame;
}

int InputChunk_get_was_transport_rolling(struct InputChunk *clip)
{
    /* Runs on the Python thread */

    return clip->was_transport_rolling;
}

int InputChunk_get_samples(struct InputChunk *clip, char *bytearray, int n)
{
    /* Runs on the Python thread */

    if (n != INPUT_CLIP_LENGTH * sizeof(jack_default_audio_sample_t))
        return 0;

    memcpy(bytearray, clip->samples, n);
    return 1;
}

void InputChunk_del(struct InputChunk *clip)
{
    /* Runs on the Python thread */

    free(clip);
}
