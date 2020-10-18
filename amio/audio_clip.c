#include "audio_clip.h"

#include <stdlib.h>
#include <string.h>
#include "communication.h"

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

void AudioClip_del(
    struct JackInterface *jack_interface, struct AudioClip *clip)
{
    /* Runs on the Python thread */

    if (!post_task_with_ptr_to_io_thread(
            &jack_interface->interface, io_thread_unref_audio_clip, clip)) {
        // TODO handle failure
    }
}
