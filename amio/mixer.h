#ifndef MIXER_H
#define MIXER_H

#include <jack/jack.h>

#include "audio_clip.h"

void add_clip_data_to_jack_port(
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    struct AudioClip *clip,
    int pos_a,
    int pos_b,
    float gain_l,
    float gain_r);

void clear_jack_port(
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    jack_nframes_t n);

void clamp_jack_port(
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    jack_nframes_t n);

#endif
