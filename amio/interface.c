#include "interface.h"

#include <stdlib.h>

#include "audio_clip.h"
#include "mixer.h"

void io_init(struct Interface *interface)
{
    interface->python_thread_queue_buffer = malloc(
        THREAD_QUEUE_SIZE * sizeof(struct Message));
    interface->io_thread_queue_buffer = malloc(
        THREAD_QUEUE_SIZE * sizeof(struct Message));
    interface->log_queue_buffer = malloc(LOG_QUEUE_SIZE * sizeof(char));
    interface->input_chunk_queue_buffer = malloc(
        INPUT_CLIP_QUEUE_SIZE * sizeof(struct InputChunk));

    PaUtil_InitializeRingBuffer(
        &interface->python_thread_queue,
        sizeof(struct Message),
        THREAD_QUEUE_SIZE,
        interface->python_thread_queue_buffer);
    PaUtil_InitializeRingBuffer(
        &interface->io_thread_queue,
        sizeof(struct Message),
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

    interface->current_playspec = NULL;
    interface->pending_playspec = NULL;

    interface->last_reported_frame_rate = -1;
    interface->last_reported_is_transport_rolling = false;
    interface->last_reported_position = -1;
}

void io_close(struct Interface *interface)
{
    free(interface->input_chunk_queue_buffer);
    free(interface->log_queue_buffer);
    free(interface->io_thread_queue_buffer);
    free(interface->python_thread_queue_buffer);
}

static int apply_pending_playspec_if_needed(
    struct Interface *state,
    int frame_in_playspec)
{
    struct Playspec *old_playspec = state->current_playspec;
    struct Playspec *new_playspec = state->pending_playspec;

    if (!new_playspec)
        return frame_in_playspec;  /* nothing to do */

    if (new_playspec == old_playspec) {
        /* Not sure how this could happen, but let's just ignore it */
        state->pending_playspec = NULL;
        return frame_in_playspec;
    }

    if (new_playspec->insert_at != -1
            && old_playspec
            && frame_in_playspec != new_playspec->insert_at)
        return frame_in_playspec;  /* We should wait and change the playspec later */

    /* Either there is no current playspec, or we already hit insert_at */

    state->current_playspec = state->pending_playspec;
    state->pending_playspec = NULL;
    if (new_playspec->insert_at != -1)
        frame_in_playspec = new_playspec->start_from;

    /* Update reference indicators */
    if (old_playspec)
        old_playspec->referenced_by_native_code = false;
    new_playspec->referenced_by_native_code = true;

    /* Destroy the old playspec if needed */
    if (old_playspec && !old_playspec->referenced_by_python) {
        if (!send_message_with_ptr(
            &state->python_thread_queue,
            MSG_DESTROY_PLAYSPEC, old_playspec)) {
            // TODO handle failure
        }
    }

    /*
     * Iterate over clips referenced by the old playspec and mark
     * them as no longer referenced.
     */
    if (old_playspec) {
        for (int i = 0; i < old_playspec->num_entries; ++i)
            if (old_playspec->entries[i].audio_clip)
                old_playspec->entries[i].
                    audio_clip->referenced_by_current_playspec = false;
    }

    /*
     * Iterate over clips referenced by the new playspec and mark
     * them as referenced.
     */
    for (int i = 0; i < new_playspec->num_entries; ++i)
        if (new_playspec->entries[i].audio_clip)
            new_playspec->entries[i].
                audio_clip->referenced_by_current_playspec = true;

    /*
     * If some clips from the old playspec are not referenced by the new
     * playspec, and also not referenced by Python, they can be destroyed.
     */
    if (old_playspec) {
        for (int i = 0; i < old_playspec->num_entries; ++i) {
            struct AudioClip *clip = old_playspec->entries[i].audio_clip;
            if (clip &&
                    !clip->referenced_by_current_playspec &&
                    !clip->referenced_by_python) {
                if (!send_message_with_ptr(
                    &state->python_thread_queue,
                    MSG_DESTROY_AUDIO_CLIP, clip)) {
                    // TODO handle failure
                }
            }
        }
    }

    return frame_in_playspec;
}

static void process_messages_on_jack_queue(
    struct Interface *state,
    struct DriverInterface *driver,
    void *driver_handle)
{
    /* Runs on the I/O thread */

    struct Message message;
    struct AudioClip *clip;

    if (PaUtil_ReadRingBuffer(&state->io_thread_queue, &message, 1) > 0) {
        switch (message.type) {
        case MSG_SET_PLAYSPEC:
            write_log(state, "I/O thread: Got MSG_SET_PLAYSPEC\n");
            state->pending_playspec = message.arg_ptr;
            break;
        case MSG_UNREF_AUDIO_CLIP:
            write_log(state, "I/O thread: Got MSG_UNREF_AUDIO_CLIP\n");
            clip = message.arg_ptr;
            clip->referenced_by_python = false;
            if (!clip->referenced_by_current_playspec) {
                if (!send_message_with_ptr(
                    &state->python_thread_queue,
                    MSG_DESTROY_AUDIO_CLIP, clip)) {
                    // TODO handle failure
                }
            }
            break;
        case MSG_SET_POS:
            write_log(state, "I/O thread: Got MSG_SET_POS\n");
            driver->set_position(driver_handle, message.arg_int);
            break;
        case MSG_SET_TRANSPORT_STATE:
            write_log(state, "I/O thread: Got MSG_SET_TRANSPORT_STATE\n");
            driver->set_is_transport_rolling(driver_handle, message.arg_int);
            break;
        default:
            abort();  /* Unknown message */
            break;
        }
    }
}

void io_process_messages_on_python_queue(struct Interface *interface)
{
    /* Runs on the Python thread */

    struct Message message;
    struct AudioClip *clip;
    struct Playspec *playspec;

    while (PaUtil_ReadRingBuffer(
            &interface->python_thread_queue, &message, 1) > 0) {
        switch (message.type) {
        case MSG_DESTROY_AUDIO_CLIP:
            clip = message.arg_ptr;
            free(clip->data);
            free(clip);
            break;
        case MSG_DESTROY_PLAYSPEC:
            playspec = message.arg_ptr;
            free(playspec->entries);
            free(playspec);
            break;
        case MSG_FRAME_RATE:
            interface->last_reported_frame_rate = message.arg_int;
            break;
        case MSG_CURRENT_POS:
            interface->last_reported_position = message.arg_int;
            break;
        case MSG_TRANSPORT_STATE:
            interface->last_reported_is_transport_rolling = message.arg_int;
            break;
        default:
            abort();  /* Unknown message */
            break;
        }
    }
}

static void mix_playspec_entry_into_jack_ports_at(
    struct PlayspecEntry *entry,
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    int a_in_playspec,
    int frame_in_playspec,
    int frames_to_copy)
{
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
            entry->audio_clip,
            a_in_clip,
            b_in_clip,
            entry->gain_l / 32768.0,
            entry->gain_r / 32768.0);
    }
}

static void mix_playspec_into_jack_ports(
    struct Interface *state,
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    int frame_in_playspec,
    int frames_to_copy)
{
    if (frames_to_copy == 0)
        return;

    for (int entry_number = 0;
         entry_number < state->current_playspec->num_entries;
         ++entry_number) {
        struct PlayspecEntry *entry = &state->current_playspec->entries[entry_number];

        if (!entry->audio_clip)
            continue;

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
    struct InputChunk clip;
    jack_nframes_t clip_i;
    jack_nframes_t buffer_i = 0;
    while (buffer_i < nframes) {
        clip.starting_frame = starting_frame + buffer_i;
        clip.was_transport_rolling = transport_state;
        for (clip_i = 0; clip_i < INPUT_CLIP_LENGTH / 2; ++clip_i, ++buffer_i) {
            clip.samples[2 * clip_i + 0] = port_l[buffer_i];
            clip.samples[2 * clip_i + 1] = port_r[buffer_i];
        }
        write_input_samples(interface, &clip);
    }
}

jack_nframes_t process_input_output_with_buffers(
    struct Interface *state,
    struct DriverInterface *driver,
    void *driver_handle,
    int frame_in_playspec,
    bool is_transport_rolling,
    jack_nframes_t nframes,
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r)
{
    /* Runs on the I/O thread */

    send_message_with_int(
        &state->python_thread_queue, MSG_CURRENT_POS, frame_in_playspec);
    send_message_with_int(
        &state->python_thread_queue, MSG_TRANSPORT_STATE, is_transport_rolling?1:0);

    clear_jack_port(port_l, port_r, nframes);

    if (!is_transport_rolling
            || !state->current_playspec
            || state->current_playspec->num_entries == 0) {
        frame_in_playspec = apply_pending_playspec_if_needed(state, frame_in_playspec);
        process_messages_on_jack_queue(state, driver, driver_handle);
        return frame_in_playspec;
    }

    jack_nframes_t frames_copied = 0;
    while (frames_copied < nframes) {
        int frames_to_copy = nframes - frames_copied;

        /*
         * Check if we'll hit the playspec change in this clip.
         * If yes, limit the number of frames to copy.
         */
        if (state->pending_playspec) {
            if (state->pending_playspec->insert_at != -1) {
                if (state->pending_playspec->insert_at - frame_in_playspec
                        < frames_to_copy)
                    frames_to_copy =
                        state->pending_playspec->insert_at - frame_in_playspec;
            } else {
                frames_to_copy = 0;  /* insert immediately */
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

        frame_in_playspec = apply_pending_playspec_if_needed(state, frame_in_playspec);
    }
    clamp_jack_port(port_l, port_r, nframes);

    process_messages_on_jack_queue(state, driver, driver_handle);

    return frame_in_playspec;
}

void io_set_playspec(struct Interface *interface,
                  struct Playspec *playspec)
{
    /* Runs on the Python thread */

    if (!send_message_with_ptr(
            &interface->io_thread_queue, MSG_SET_PLAYSPEC, playspec)) {
        // TODO handle failure
    }
}

int io_get_frame_rate(struct Interface *interface)
{
    /* Runs on the Python thread */

    return interface->last_reported_frame_rate;
}

int io_get_position(struct Interface *interface)
{
    /* Runs on the Python thread */

    return interface->last_reported_position;
}

void io_set_position(struct Interface *interface, int position)
{
    /* Runs on the Python thread */

    send_message_with_int(
        &interface->io_thread_queue, MSG_SET_POS, position);
}

int io_get_transport_rolling(struct Interface *interface)
{
    /* Runs on the Python thread */

    return interface->last_reported_is_transport_rolling;
}

void io_set_transport_rolling(struct Interface *interface, int rolling)
{
    /* Runs on the Python thread */

    send_message_with_int(
        &interface->io_thread_queue, MSG_SET_TRANSPORT_STATE, rolling);
}
