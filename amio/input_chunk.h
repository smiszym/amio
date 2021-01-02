#ifndef INPUT_CLIP_H
#define INPUT_CLIP_H

#include <jack/jack.h>

#define INPUT_CLIP_LENGTH 128

struct InputChunk
{
    int playspec_id;
    int starting_frame;
    int was_transport_rolling;
    jack_default_audio_sample_t samples[INPUT_CLIP_LENGTH];
};

extern struct InputChunk input_chunk_being_read;

int InputChunk_get_playspec_id();
int InputChunk_get_starting_frame();
int InputChunk_get_was_transport_rolling();
int InputChunk_get_samples(char *bytearray, int n);

#endif
