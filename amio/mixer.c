#include "mixer.h"

void add_clip_data_to_jack_port(
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    struct AudioClip *clip,
    int pos_a,
    int pos_b,
    float gain_l,
    float gain_r)
{
    if (clip->channels >= 2) {
        for (int n = pos_a * clip->channels;
                 n < pos_b * clip->channels;
                 n += clip->channels) {
            *port_l++ += clip->data[n + 0] * gain_l;
            *port_r++ += clip->data[n + 1] * gain_r;
        }
    } else if (clip->channels == 1) {
        for (int n = pos_a; n < pos_b; ++n) {
            *port_l++ += clip->data[n] * gain_l;
            *port_r++ += clip->data[n] * gain_r;
        }
    }
}

void clear_jack_port(
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    jack_nframes_t n)
{
    for (jack_nframes_t i = 0; i < n; ++i) {
        *port_l++ = 0.0;
        *port_r++ = 0.0;
    }
}

void clamp_jack_port(
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    jack_nframes_t n)
{
    for (jack_nframes_t i = 0; i < n; ++i, ++port_l, ++port_r) {
        if (*port_l >= 1.0)
            *port_l = 1.0;
        else if (*port_l <= -1.0)
            *port_l = -1.0;

        if (*port_r >= 1.0)
            *port_r = 1.0;
        else if (*port_r <= -1.0)
            *port_r = -1.0;
    }
}
