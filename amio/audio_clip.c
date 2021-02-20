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

    struct AudioClip *clip = get_audio_clip_by_id(clip_id);
    if (!clip)
        return;

    clip->referenced_by_python = false;
}

void destroy_audio_clip(int audio_clip_id)
{
    /* Runs on the Python thread */

    struct AudioClip *clip = get_audio_clip_by_id(audio_clip_id);
    if (!clip)
        return;

    pool_remove(pool, audio_clip_id);
    free(clip->data);
    free(clip);
}

void for_each_audio_clip(void (*callback)(int audio_clip_id))
{
    ensure_pool_initialized();
    pool_for_each(pool, callback);
}
