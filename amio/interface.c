#include "interface.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "audio_clip.h"
#include "gc.h"
#include "mixer.h"
#include "pool.h"

#include "jack_driver.h"

static struct Pool *pool;

static void ensure_pool_initialized()
{
    /* Runs on the Python thread */

    if (!pool) {
        pool = malloc(sizeof(struct Pool));
        pool_create(pool, MAX_INTERFACES);
    }
}

struct Interface * get_interface_by_id(int id)
{
    /* Runs on the Python thread */

    ensure_pool_initialized();
    return pool_find(pool, id);
}

int iface_get_key(int interface_id)
{
    /* Runs on the Python thread */

    ensure_pool_initialized();
    return pool_get_key(pool, interface_id);
}

void for_each_interface(void (*callback)(int interface_id))
{
    ensure_pool_initialized();
    pool_for_each(pool, callback);
}

int create_interface(struct Driver *driver, const char *client_name)
{
    /* Runs on the Python thread */

    ensure_pool_initialized();

    struct Interface *interface = malloc(sizeof(struct Interface));
    interface->id = pool_put(pool, interface);
    interface->driver = driver;
    interface->driver_state = driver->create_state_object(
        client_name, interface);

    interface->python_thread_queue_buffer = malloc(
        THREAD_QUEUE_SIZE * sizeof(struct Task));
    interface->io_thread_queue_buffer = malloc(
        THREAD_QUEUE_SIZE * sizeof(struct Task));
    interface->log_queue_buffer = malloc(LOG_QUEUE_SIZE * sizeof(char));
    interface->input_chunk_queue_buffer = malloc(
        INPUT_CLIP_QUEUE_SIZE * sizeof(struct InputChunk));

    PaUtil_InitializeRingBuffer(
        &interface->python_thread_queue,
        sizeof(struct Task),
        THREAD_QUEUE_SIZE,
        interface->python_thread_queue_buffer);
    PaUtil_InitializeRingBuffer(
        &interface->io_thread_queue,
        sizeof(struct Task),
        THREAD_QUEUE_SIZE,
        interface->io_thread_queue_buffer);
    PaUtil_InitializeRingBuffer(
        &interface->log_queue,
        sizeof(char),
        LOG_QUEUE_SIZE,
        interface->log_queue_buffer);
    PaUtil_InitializeRingBuffer(
        &interface->input_chunk_queue,
        sizeof(struct InputChunk),
        INPUT_CLIP_QUEUE_SIZE,
        interface->input_chunk_queue_buffer);

    interface->py_thread_current_playspec = create_empty_playspec();
    interface->py_thread_pending_playspec = NULL;

    interface->current_playspec = interface->py_thread_current_playspec;
    interface->pending_playspec = NULL;

    interface->last_reported_frame_rate = -1;
    interface->last_reported_is_transport_rolling = false;
    interface->last_reported_position = -1;

    driver->init(interface->driver_state);
    return interface->id;
}

int create_jack_interface(const char *client_name)
{
    /* Runs on the Python thread */

    return create_interface(&jack_driver, client_name);
}

void iface_close(int interface_id)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);

    interface->driver->destroy(interface->driver_state);
    free(interface->input_chunk_queue_buffer);
    free(interface->log_queue_buffer);
    free(interface->io_thread_queue_buffer);
    free(interface->python_thread_queue_buffer);
}

static int py_thread_on_playspec_applied(
    struct Interface *interface, union TaskArgument arg)
{
    /* Runs on the Python thread */

    struct Playspec *old_playspec = interface->py_thread_current_playspec;
    struct Playspec *new_playspec = arg.pointer;

    assert(old_playspec != new_playspec);

    if (old_playspec) {
        free(old_playspec->entries);
        free(old_playspec);
    }

    assert(new_playspec == interface->py_thread_pending_playspec);

    interface->py_thread_current_playspec = new_playspec;
    interface->py_thread_pending_playspec = NULL;

    gc_audio_clips();
    return PY_QUEUE_PROCESSING_RESULT_PLAYSPEC_APPLIED;
}

static int py_thread_receive_current_pos(
    struct Interface *interface, union TaskArgument arg)
{
    /* Runs on the Python thread */

    interface->last_reported_position = arg.integer;
    return 0;
}

static int py_thread_receive_transport_state(
    struct Interface *interface, union TaskArgument arg)
{
    /* Runs on the Python thread */

    interface->last_reported_is_transport_rolling = arg.integer;
    return 0;
}

static int apply_pending_playspec_if_needed(
    struct Interface *state,
    int frame_in_playspec,
    int start_from_offset)
{
    /* Runs on the I/O thread */

    struct Playspec *old_playspec = state->current_playspec;
    struct Playspec *new_playspec = state->pending_playspec;

    if (!new_playspec)
        return frame_in_playspec;  /* nothing to do */

    assert(old_playspec != new_playspec);

    if (old_playspec && frame_in_playspec < new_playspec->insert_at)
        return frame_in_playspec;  /* We should wait and change the playspec later */

    /* Either there is no current playspec, or we already hit insert_at */

    state->current_playspec = state->pending_playspec;
    state->pending_playspec = NULL;
    frame_in_playspec = new_playspec->start_from + start_from_offset;

    /* Update reference indicators */
    if (old_playspec)
        old_playspec->referenced_by_native_code = false;
    new_playspec->referenced_by_native_code = true;

    /* Notify the Python thread that the playspec was applied */
    if (!post_task_with_ptr_to_py_thread(
        state, py_thread_on_playspec_applied, new_playspec)) {
        // TODO handle failure
    }

    return frame_in_playspec;
}

static void io_thread_set_playspec(
    struct Interface *state, struct Driver *driver,
    void *driver_handle, union TaskArgument arg)
{
    /* Runs on the I/O thread */

    write_log(state, "I/O thread: Got MSG_SET_PLAYSPEC\n");
    state->pending_playspec = arg.pointer;
}

static void io_thread_set_pos(
    struct Interface *state, struct Driver *driver,
    void *driver_handle, union TaskArgument arg)
{
    /* Runs on the I/O thread */

    write_log(state, "I/O thread: Got MSG_SET_POS\n");
    driver->set_position(driver_handle, arg.integer);
}

static void io_thread_set_transport_state(
    struct Interface *state, struct Driver *driver,
    void *driver_handle, union TaskArgument arg)
{
    /* Runs on the I/O thread */

    write_log(state, "I/O thread: Got MSG_SET_TRANSPORT_STATE\n");
    driver->set_is_transport_rolling(driver_handle, arg.integer);
}

static void process_messages_on_jack_queue(
    struct Interface *state,
    struct Driver *driver,
    void *driver_handle)
{
    /* Runs on the I/O thread */

    struct Task message;
    if (PaUtil_ReadRingBuffer(&state->io_thread_queue, &message, 1) > 0) {
        message.callable.io_thread_callable(
            state, driver, driver_handle, message.arg);
    }
}

int iface_process_messages_on_python_queue(int interface_id)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);

    struct Task message;
    while (PaUtil_ReadRingBuffer(
            &interface->python_thread_queue, &message, 1) > 0) {
        int res = message.callable.py_thread_callable(interface, message.arg);
        if (res)
            return res;
    }
    return 0;
}

static void mix_playspec_entry_into_jack_ports_at(
    struct PlayspecEntry *entry,
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    int a_in_playspec,
    int frame_in_playspec,
    int frames_to_copy)
{
    /* Runs on the I/O thread */

    struct AudioClip *clip = get_audio_clip_by_id(entry->audio_clip_id);

    if (!clip)
        return;

    int a_in_clip = entry->clip_frame_a;
    int b_in_clip = entry->clip_frame_b;

    int b_in_playspec = a_in_playspec + (b_in_clip - a_in_clip);

    /* Clamp playspec positions */
    if (a_in_playspec < frame_in_playspec) {
        int delta = frame_in_playspec - a_in_playspec;
        a_in_playspec += delta;
        a_in_clip += delta;
    }
    if (b_in_playspec > frame_in_playspec + frames_to_copy) {
        int delta = b_in_playspec - (frame_in_playspec + frames_to_copy);
        b_in_playspec -= delta;
        b_in_clip -= delta;
    }

    if (a_in_playspec < b_in_playspec && a_in_playspec < frame_in_playspec + frames_to_copy) {
        int delta = a_in_playspec - frame_in_playspec;
        add_clip_data_to_jack_port(
            port_l + delta, port_r + delta,
            clip,
            a_in_clip,
            b_in_clip,
            entry->gain_l,
            entry->gain_r);
    }
}

static void mix_playspec_into_jack_ports(
    struct Interface *state,
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    int frame_in_playspec,
    int frames_to_copy)
{
    /* Runs on the I/O thread */

    if (!state->current_playspec)
        return;

    if (frames_to_copy == 0)
        return;

    for (int entry_number = 0;
         entry_number < state->current_playspec->num_entries;
         ++entry_number) {
        struct PlayspecEntry *entry = &state->current_playspec->entries[entry_number];

        if (entry->repeat_interval == 0) {
            /* No repetitions. */

            mix_playspec_entry_into_jack_ports_at(
                entry, port_l, port_r, entry->play_at_frame,
                frame_in_playspec, frames_to_copy);
        } else {
            /* Periodic playspec entry. */

            /* Make sure 0 <= play_at_frame < repeat_interval */
            int frame = entry->play_at_frame;
            int interval = entry->repeat_interval;
            int play_at_frame = frame - (frame / interval * interval);

            /* Find the last repetition that falls into the range */
            int end_frame = frame_in_playspec + frames_to_copy;
            int a_in_playspec = end_frame / interval * interval + play_at_frame;

            int clip_length = entry->clip_frame_b - entry->clip_frame_a;
            while (a_in_playspec + clip_length >= frame_in_playspec) {
                mix_playspec_entry_into_jack_ports_at(
                    entry, port_l, port_r, a_in_playspec,
                    frame_in_playspec, frames_to_copy);
                a_in_playspec -= interval;
            }
        }
    }
}

void process_input_with_buffers(
    struct Interface *interface,
    jack_nframes_t nframes,
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    int starting_frame,
    int transport_state)
{
    /* Runs on the I/O thread */

    struct InputChunk clip;
    jack_nframes_t clip_i;
    jack_nframes_t buffer_i = 0;
    while (buffer_i < nframes) {
        clip.playspec_id = interface->current_playspec->id;
        clip.starting_frame = starting_frame + buffer_i;
        clip.was_transport_rolling = transport_state;
        for (clip_i = 0; clip_i < INPUT_CLIP_LENGTH / 2; ++clip_i, ++buffer_i) {
            clip.samples[2 * clip_i + 0] = port_l[buffer_i];
            clip.samples[2 * clip_i + 1] = port_r[buffer_i];
        }
        write_input_samples(interface, &clip);
    }
}

jack_nframes_t process_output_with_buffers(
    struct Interface *state,
    int frame_in_playspec,
    bool is_transport_rolling,
    jack_nframes_t nframes,
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r)
{
    /* Runs on the I/O thread */

    post_task_with_int_to_py_thread(
        state, py_thread_receive_current_pos, frame_in_playspec);
    post_task_with_int_to_py_thread(
        state, py_thread_receive_transport_state, is_transport_rolling?1:0);

    clear_jack_port(port_l, port_r, nframes);

    if (!is_transport_rolling) {
        int start_from_offset = 0;
        if (state->pending_playspec
                && frame_in_playspec > state->pending_playspec->insert_at)
            start_from_offset =
                frame_in_playspec - state->pending_playspec->insert_at;
        frame_in_playspec = apply_pending_playspec_if_needed(
            state, frame_in_playspec, start_from_offset);
        process_messages_on_jack_queue(
            state, state->driver, state->driver_state);
        return frame_in_playspec;
    }

    jack_nframes_t frames_copied = 0;
    while (frames_copied < nframes) {
        int frames_to_copy = nframes - frames_copied;
        int start_from_offset = 0;

        /*
         * Check if we'll hit the playspec change in this clip.
         * If yes, limit the number of frames to copy.
         */
        if (state->pending_playspec) {
            int ahead_by =
                state->pending_playspec->insert_at - frame_in_playspec;
            if (frames_to_copy > ahead_by)
                frames_to_copy = ahead_by;
            if (frames_to_copy < 0) {
                start_from_offset = -frames_to_copy;
                frames_to_copy = 0;
            }
        }

        mix_playspec_into_jack_ports(
            state,
            port_l + frames_copied,
            port_r + frames_copied,
            frame_in_playspec,
            frames_to_copy);
        frames_copied += frames_to_copy;
        frame_in_playspec += frames_to_copy;

        frame_in_playspec = apply_pending_playspec_if_needed(
            state, frame_in_playspec, start_from_offset);
    }
    clamp_jack_port(port_l, port_r, nframes);

    process_messages_on_jack_queue(state, state->driver, state->driver_state);

    return frame_in_playspec;
}

int iface_set_playspec(int interface_id)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);
    struct Playspec *playspec = get_built_playspec();

    if (interface->py_thread_pending_playspec)
        return -1;

    interface->py_thread_pending_playspec = playspec;

    if (!post_task_with_ptr_to_io_thread(
            interface, io_thread_set_playspec, playspec)) {
        interface->py_thread_pending_playspec = NULL;
        return -1;
    }

    return playspec->id;
}

int iface_get_frame_rate(int interface_id)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);
    return interface->last_reported_frame_rate;
}

int iface_get_position(int interface_id)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);
    return interface->last_reported_position;
}

void iface_set_position(int interface_id, int position)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);
    post_task_with_int_to_io_thread(interface, io_thread_set_pos, position);
}

int iface_get_transport_rolling(int interface_id)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);
    return interface->last_reported_is_transport_rolling;
}

void iface_set_transport_rolling(int interface_id, int rolling)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);
    post_task_with_int_to_io_thread(
        interface, io_thread_set_transport_state, rolling);
}

int iface_get_current_playspec_id(int interface_id)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);
    if (!interface)
        return -1;

    struct Playspec *playspec = interface->current_playspec;
    if (!playspec)
        return -1;

    return playspec->id;
}
