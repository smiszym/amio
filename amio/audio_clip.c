#include "audio_clip.h"

#include <stdlib.h>
#include <string.h>

#include "interface.h"
#include "pool.h"

#define MAX_AUDIO_CLIPS 1024

static struct Pool *pool;

static void ensure_pool_initialized()
{
    if (!pool) {
        pool = malloc(sizeof(struct Pool));
        pool_create(pool, MAX_AUDIO_CLIPS);
    }
}

struct AudioClip * get_audio_clip_by_id(int id)
{
    ensure_pool_initialized();
    return pool_find(pool, id);
}

int AudioClip_init(char *bytes, int n, int channels, float framerate)
{
    /* Runs on the Python thread */

    ensure_pool_initialized();

    struct AudioClip *result;
    result = malloc(sizeof(struct AudioClip));
    result->id = pool_put(pool, result);
    result->referenced_by_python = true;
    result->length = n / (sizeof(int16_t) * channels);
    result->channels = channels;
    result->framerate = framerate;
    result->data = malloc(n);
    memcpy(result->data, bytes, n);
    return result->id;
}

void AudioClip_del(int interface, int clip_id)
{
    /* Runs on the Python thread */

    if (!post_task_with_int_to_io_thread(
            get_interface_by_id(interface), io_thread_unref_audio_clip, clip_id)) {
        // TODO handle failure
    }
}

void io_thread_unref_audio_clip(
    struct Interface *state, struct Driver *driver,
    void *driver_handle, union TaskArgument arg)
{
    write_log(state, "I/O thread: Got MSG_UNREF_AUDIO_CLIP\n");
    int clip_id = arg.integer;

    struct AudioClip *clip = get_audio_clip_by_id(clip_id);
    if (!clip)
        return;

    pool_remove(pool, clip_id);
    clip->referenced_by_python = false;
    if (!clip->referenced_by_current_playspec) {
        if (!post_task_with_ptr_to_py_thread(
                state, py_thread_destroy_audio_clip, clip)) {
            // TODO handle failure
        }
    }
}
