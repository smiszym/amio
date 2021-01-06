#include "playspec.h"

#include "stddef.h"
#include <stdlib.h>

struct Playspec *playspec_being_built = NULL;

bool begin_defining_playspec(int size, int insert_at, int start_from)
{
    /* Runs on the Python thread */

    if (playspec_being_built)
        return false;

    playspec_being_built = malloc(sizeof(struct Playspec));
    playspec_being_built->num_entries = size;
    playspec_being_built->entries = malloc(
        size * sizeof(struct PlayspecEntry));

    for (int i = 0; i < size; ++i) {
        playspec_being_built->entries[i].audio_clip = NULL;
        playspec_being_built->entries[i].clip_frame_a = 0;
        playspec_being_built->entries[i].clip_frame_b = 0;
        playspec_being_built->entries[i].play_at_frame = 0;
        playspec_being_built->entries[i].gain_l = 1.0;
        playspec_being_built->entries[i].gain_r = 1.0;
    }

    playspec_being_built->insert_at = insert_at;
    playspec_being_built->start_from = start_from;

    return true;
}

void set_entry_in_playspec(
    int n,
    struct AudioClip *clip,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r)
{
    /* Runs on the Python thread */

    if (n < 0 || n >= playspec_being_built->num_entries)
        return;

    playspec_being_built->entries[n].audio_clip = clip;
    playspec_being_built->entries[n].clip_frame_a = clip_frame_a;
    playspec_being_built->entries[n].clip_frame_b = clip_frame_b;
    playspec_being_built->entries[n].play_at_frame = play_at_frame;
    playspec_being_built->entries[n].repeat_interval = repeat_interval;
    playspec_being_built->entries[n].gain_l = gain_l;
    playspec_being_built->entries[n].gain_r = gain_r;
}
