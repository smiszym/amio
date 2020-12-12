#include "audio_clip.h"

#include <stdlib.h>
#include <string.h>

#include "interface.h"

struct AudioClip * AudioClip_init(
    char *bytes, int n, int channels, float framerate)
{
    /* Runs on the Python thread */

    struct AudioClip *result;
    result = malloc(sizeof(struct AudioClip));
    result->referenced_by_python = true;
    result->length = n / (sizeof(int16_t) * channels);
    result->channels = channels;
    result->framerate = framerate;
    result->data = (int16_t *)malloc(n);
    memcpy(result->data, bytes, n);
    return result;
}

void AudioClip_del(int interface, struct AudioClip *clip)
{
    /* Runs on the Python thread */

    if (!post_task_with_ptr_to_io_thread(
            get_interface_by_id(interface), io_thread_unref_audio_clip, clip)) {
        // TODO handle failure
    }
}

void io_thread_unref_audio_clip(
    struct Interface *state, struct DriverInterface *driver,
    void *driver_handle, union TaskArgument arg)
{
    write_log(state, "I/O thread: Got MSG_UNREF_AUDIO_CLIP\n");
    struct AudioClip *clip = arg.pointer;
    clip->referenced_by_python = false;
    if (!clip->referenced_by_current_playspec) {
        if (!post_task_with_ptr_to_py_thread(
                state, py_thread_destroy_audio_clip, clip)) {
            // TODO handle failure
        }
    }
}
