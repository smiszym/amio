#include "gc.h"

#include <assert.h>
#include <stdbool.h>

#include "audio_clip.h"
#include "interface.h"

static void prepare(int audio_clip_id)
{
    struct AudioClip *clip = get_audio_clip_by_id(audio_clip_id);
    for (int i = 0; i < MAX_INTERFACES; ++i)
        clip->referenced_by_io_thread[i] = false;
}

static void mark_clips_from_playspec(
    struct Playspec *playspec, int interface_id)
{
    if (!playspec)
        return;

    for (int i = 0; i < playspec->num_entries; ++i) {
        struct AudioClip *clip = get_audio_clip_by_id(
            playspec->entries[i].audio_clip_id);
        if (clip) {
            clip->referenced_by_io_thread[iface_get_key(interface_id)] = true;
        }
    }
}

static void mark(int interface_id)
{
    struct Interface *interface = get_interface_by_id(interface_id);
    mark_clips_from_playspec(
        interface->py_thread_current_playspec, interface_id);
    mark_clips_from_playspec(
        interface->py_thread_pending_playspec, interface_id);
}

static void sweep(int audio_clip_id)
{
    struct AudioClip *clip = get_audio_clip_by_id(audio_clip_id);

    if (clip->referenced_by_python)
        return;

    for (int i = 0; i < MAX_INTERFACES; ++i)
        if (clip->referenced_by_io_thread[i])
            return;

    destroy_audio_clip(audio_clip_id);
}

void gc_audio_clips()
{
    for_each_audio_clip(prepare);
    for_each_interface(mark);
    for_each_audio_clip(sweep);
}
