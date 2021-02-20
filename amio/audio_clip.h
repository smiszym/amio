#ifndef AUDIO_CLIP_H
#define AUDIO_CLIP_H

#include <stdbool.h>
#include <stdint.h>

#include "communication.h"
#include "interface.h"

/*
 * AudioClip objects are created on the Python thread. When they are fully
 * initialized, pointers to them can be passed to the I/O thread.
 *
 * After a pointer to AudioClip is passed to the I/O thread, the members
 * can only be read or written by the I/O thread.
 *
 * When the I/O thread changes the current playspec in response to
 * a MSG_SET_PLAYSPEC message, and an AudioClip stops being referenced
 * by the current playspec, its referenced_by_current_playspec is set to false.
 * If, at that time, referenced_by_python is also false, the I/O thread
 * schedules deletion of AudioClip object on the Python thread by posting
 * a MSG_DESTROY_AUDIO_CLIP message.
 *
 * Similarly, when an AudioClip starts being referenced by the current playspec,
 * its referenced_by_current_playspec is set to true.
 *
 * referenced_by_python is initially true, when the object is created on
 * the Python thread, and is set to false on the I/O thread upon arrival
 * of a MSG_UNREF_AUDIO_CLIP message.
 */
struct AudioClip
{
    int id;

    int length;
    int channels;
    int framerate;
    int16_t *data;

    /* The following fields are only accessed from the Python thread */

    /*
     * Indicator whether the playspec currently in the playspec queue
     * on the I/O thread might have a reference to this clip
     */
    bool referenced_by_io_thread[MAX_INTERFACES];
    /* Indicator whether Python has a reference to this clip */
    bool referenced_by_python;
};

struct AudioClip * get_audio_clip_by_id(int id);

int AudioClip_init(char *bytes, int n, int channels, float framerate);
void AudioClip_del(int interface, int clip_id);

void destroy_audio_clip(int audio_clip_id);

void for_each_audio_clip(void (*callback)(int audio_clip_id));

#endif
