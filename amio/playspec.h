#ifndef PLAYSPEC_H
#define PLAYSPEC_H

#include <stdbool.h>
#include <stdint.h>

struct JackInterface;
struct Interface;

struct PlayspecEntry
{
    /* Audio clip to mix into the output */
    struct AudioClip *audio_clip;

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

    /* Playspec length in frames. The playspec is looped. */
    int length;

    /*
     * Time in frames, counted from the beginning of the previous
     * playspec, at which to insert this playspec. Only read by the JACK
     * thread when swapping playspecs.
     *
     * A special value of -1 means to insert immediately and start from
     * this same position in the new playspec (start_from is ignored
     * in that case).
     */
    int insert_at;

    /*
     * Time in frames, counted from the beginning of this playspec,
     * at which to start playing this playspec. Only read by the JACK
     * thread when swapping playspecs.
     */
    int start_from;

    /* Indicator whether Python has a reference to this playspec */
    bool referenced_by_python;
    /*
     * Playspec is referenced by the native code since it's passed
     * to it in MSG_SET_PLAYSPEC message, through replacing
     * the previous playspec, until being replaced itself
     * by a new playspec.
     */
    bool referenced_by_native_code;
};

struct Playspec * Playspec_init(int size);
void Playspec_del(
    struct JackInterface *jack_interface, struct Playspec *playspec);
void Playspec_setEntry(struct Playspec *playspec, int n,
    struct AudioClip *clip,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);
void Playspec_setLength(struct Playspec *playspec, int length);
void Playspec_setInsertionPoints(
    struct Playspec *playspec, int insert_at, int start_from);

#endif
