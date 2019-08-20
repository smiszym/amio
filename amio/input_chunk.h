#ifndef INPUT_CLIP_H
#define INPUT_CLIP_H

#include <jack/jack.h>

#define INPUT_CLIP_LENGTH 128

struct InputChunk
{
    int starting_frame;
    int was_transport_rolling;
    jack_default_audio_sample_t samples[INPUT_CLIP_LENGTH];
};

int InputChunk_get_starting_frame(struct InputChunk *clip);
int InputChunk_get_was_transport_rolling(struct InputChunk *clip);
int InputChunk_get_samples(struct InputChunk *clip, char *bytearray, int n);
void InputChunk_del(struct InputChunk *clip);

#endif
