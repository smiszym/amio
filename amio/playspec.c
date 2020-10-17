#include "playspec.h"

#include "stddef.h"
#include <stdlib.h>

struct Playspec * Playspec_init(int size)
{
    /* Runs on the Python thread */

    struct Playspec *result;
    result = malloc(sizeof(struct Playspec));
    result->referenced_by_python = true;
    result->num_entries = size;
    result->entries = malloc(
        size * sizeof(struct PlayspecEntry));

    for (int i = 0; i < size; ++i) {
        result->entries[i].audio_clip = NULL;
        result->entries[i].clip_frame_a = 0;
        result->entries[i].clip_frame_b = 0;
        result->entries[i].play_at_frame = 0;
        result->entries[i].gain_l = 1.0;
        result->entries[i].gain_r = 1.0;
    }

    result->insert_at = -1;
    result->start_from = 0;

    return result;
}

void Playspec_del(
    struct JackInterface *jack_interface, struct Playspec *playspec)
{
    /* Runs on the Python thread */

    playspec->referenced_by_python = false;
    // TODO Set this field on the I/O thread
}

void Playspec_setEntry(struct Playspec *playspec, int n,
    struct AudioClip *clip,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r)
{
    /* Runs on the Python thread */

    /*
     * Calling this is illegal after a reference to this playspec
     * was passed to the Python thread (e.g., in MSG_SET_PLAYSPEC
     * message).
     */

    if (n < 0 || n >= playspec->num_entries)
        return;

    playspec->entries[n].audio_clip = clip;
    playspec->entries[n].clip_frame_a = clip_frame_a;
    playspec->entries[n].clip_frame_b = clip_frame_b;
    playspec->entries[n].play_at_frame = play_at_frame;
    playspec->entries[n].repeat_interval = repeat_interval;
    playspec->entries[n].gain_l = gain_l;
    playspec->entries[n].gain_r = gain_r;
}

void Playspec_setInsertionPoints(
    struct Playspec *playspec, int insert_at, int start_from)
{
    /* Runs on the Python thread */

    /*
     * Calling this is illegal after a reference to this playspec
     * was passed to the Python thread (e.g., in MSG_SET_PLAYSPEC
     * message).
     */

    playspec->insert_at = insert_at;
    playspec->start_from = start_from;
}
