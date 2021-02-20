#ifndef PLAYSPEC_H
#define PLAYSPEC_H

#include <stdbool.h>
#include <stdint.h>

struct PlayspecEntry
{
    /* Audio clip to mix into the output */
    int audio_clip_id;

    /* Beginning and end of the clip region to use (in frames) */
    int clip_frame_a;
    int clip_frame_b;

    /*
     * Position at which to play this clip (in frames),
     * counted from the beginning of the playspec to the place
     * where the region to play starts.
     */
    int play_at_frame;

    /*
     * If non-zero, the clip fragment will appear periodically in the output.
     */
    int repeat_interval;

    /*
     * Gain of left and right channels, expressed as a factor
     * (0.0 - silence, 1.0 - original volume)
     */
    float gain_l;
    float gain_r;
};

struct Playspec
{
    int num_entries;
    struct PlayspecEntry *entries;

    /*
     * Playspec tracking id.
     */
    int id;

    /*
     * Time in frames, counted from the beginning of the previous
     * playspec, at which to insert this playspec. Only read by the JACK
     * thread when swapping playspecs.
     */
    int insert_at;

    /*
     * Time in frames, counted from the beginning of this playspec,
     * at which to start playing this playspec. Only read by the JACK
     * thread when swapping playspecs.
     */
    int start_from;

    /*
     * Playspec is referenced by the native code since it's passed
     * to it in MSG_SET_PLAYSPEC message, through replacing
     * the previous playspec, until being replaced itself
     * by a new playspec.
     */
    bool referenced_by_native_code;
};

/* API for Python code */

bool begin_defining_playspec(int size, int insert_at, int start_from);
void set_entry_in_playspec(
    int n,
    int clip_id,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);

/* API for C code */

struct Playspec * get_built_playspec();
struct Playspec * create_empty_playspec();

#endif
