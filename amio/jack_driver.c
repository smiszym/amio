#include "jack_driver.h"

#include <jack/jack.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio_clip.h"
#include "interface.h"
#include "communication.h"
#include "mixer.h"
#include "interface.h"
#include "playspec.h"

struct JackDriverState
{
    struct Interface *interface;

    char *client_name;

    jack_client_t *client;
    jack_port_t *input_port_l;
    jack_port_t *input_port_r;
    jack_port_t *output_port_l;
    jack_port_t *output_port_r;

    int total_latency;

    bool is_transport_rolling;
    int frame_in_playspec;  /* position in the whole playspec */
};

static void jack_iface_init(void *driver_state);

static void * jack_create_state_object(
    const char *client_name, struct Interface *interface)
{
    struct JackDriverState *state = malloc(sizeof(struct JackDriverState));

    state->interface = interface;

    state->client_name = strdup(client_name);
    state->client = NULL;
    state->input_port_l = NULL;
    state->input_port_r = NULL;
    state->output_port_l = NULL;
    state->output_port_r = NULL;

    state->total_latency = 0;

    state->is_transport_rolling = false;
    state->frame_in_playspec = 0;
    return state;
}

static void jack_destroy(void *driver_state)
{
    struct JackDriverState *state = driver_state;
    jack_client_close(state->client);
    free(state);
}

static void jack_set_position(void *driver_state, int position)
{
    /* Runs on the I/O thread */
    struct JackDriverState *state = driver_state;
    state->frame_in_playspec = position;
}

static void jack_set_is_transport_rolling(void *driver_state, bool value)
{
    /* Runs on the I/O thread */
    struct JackDriverState *state = driver_state;
    state->is_transport_rolling = value;
}

static int process(jack_nframes_t nframes, void *arg)
{
    /* Runs on the I/O thread */

    struct JackDriverState *state = arg;

    jack_default_audio_sample_t *in_buffer_l, *in_buffer_r;
    jack_default_audio_sample_t *out_buffer_l, *out_buffer_r;

    in_buffer_l = (jack_default_audio_sample_t*)jack_port_get_buffer(
        state->input_port_l, nframes);
    in_buffer_r = (jack_default_audio_sample_t*)jack_port_get_buffer(
        state->input_port_r, nframes);

    process_input_with_buffers(
        state->interface,
        nframes,
        in_buffer_l,
        in_buffer_r,
        state->frame_in_playspec - state->total_latency,
        state->is_transport_rolling);

    out_buffer_l = (jack_default_audio_sample_t*)jack_port_get_buffer(
        state->output_port_l, nframes);
    out_buffer_r = (jack_default_audio_sample_t*)jack_port_get_buffer(
        state->output_port_r, nframes);

    int old_frame = state->frame_in_playspec;

    int new_frame = process_output_with_buffers(
        state->interface,
        state->frame_in_playspec,
        state->is_transport_rolling,
        nframes, out_buffer_l, out_buffer_r);

    /* Only advance the current position if it wasn't changed from Python. */
    if (state->frame_in_playspec == old_frame)
        state->frame_in_playspec = new_frame;

    return 0;
}

static void jack_shutdown(void *arg)
{
    struct JackDriverState *state = arg;
    iface_close(state->interface->id);
}

static void jack_iface_init(void *driver_state)
{
    /* Runs on the Python thread */

    struct JackDriverState *state = driver_state;

    const char **ports;
    jack_options_t options = JackNullOption;
    jack_status_t status;

    state->is_transport_rolling = false;
    state->frame_in_playspec = 0;

    state->client = jack_client_open(
        state->client_name, options, &status, NULL);
    if (state->client == NULL) {
        write_log(state->interface, "jack_client_open() failed\n");
        if (status & JackServerFailed) {
            write_log(state->interface, "Unable to connect to JACK server\n");
        }
        iface_close(state->interface->id);
        return;
    }
    if (status & JackServerStarted) {
        write_log(state->interface, "JACK server started\n");
    }
    if (status & JackNameNotUnique) {
        state->client_name =
            jack_get_client_name(state->client);
        write_log(state->interface, "Unique name assigned: ");
        write_log(state->interface, state->client_name);
        write_log(state->interface, "\n");
    }

    state->interface->last_reported_frame_rate = jack_get_sample_rate(
        state->client);

    jack_set_process_callback(
        state->client, process, state);
    jack_on_shutdown(state->client, jack_shutdown, state);

    state->input_port_l = jack_port_register(
        state->client, "input_l",
        JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsInput, 0);

    state->input_port_r = jack_port_register(
        state->client, "input_r",
        JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsInput, 0);

    if ((state->input_port_l == NULL)
            || (state->input_port_r == NULL)) {
        write_log(state->interface, "No more JACK ports available\n");
        iface_close(state->interface->id);
        return;
    }

    state->output_port_l = jack_port_register(
        state->client, "output_l",
        JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsOutput, 0);

    state->output_port_r = jack_port_register(
        state->client, "output_r",
        JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsOutput, 0);

    if ((state->output_port_l == NULL)
            || (state->output_port_r == NULL)) {
        write_log(state->interface, "No more JACK ports available\n");
        iface_close(state->interface->id);
        return;
    }

    if (jack_activate(state->client)) {
        write_log(state->interface, "Cannot activate JACK client\n");
        iface_close(state->interface->id);
        return;
    }

    ports = jack_get_ports(state->client, NULL, NULL,
                           JackPortIsPhysical|JackPortIsOutput);
    if (ports == NULL) {
        write_log(state->interface, "No physical capture ports\n");
        iface_close(state->interface->id);
        return;
    }

    if (jack_connect(state->client,
            ports[0], jack_port_name(state->input_port_l))) {
        write_log(state->interface, "Cannot connect input ports\n");
    }

    if (jack_connect(state->client,
            ports[1], jack_port_name(state->input_port_r))) {
        write_log(state->interface, "Cannot connect input ports\n");
    }

    /*
     * Calculating the total latency only for one channel and assuming
     * the other channel has exactly the same latency.
     */
    jack_latency_range_t capture_latency;
    jack_port_get_latency_range(
        jack_port_by_name(state->client, ports[0]),
        JackCaptureLatency,
        &capture_latency);

    jack_free(ports);

    ports = jack_get_ports(state->client, NULL, NULL,
                           JackPortIsPhysical|JackPortIsInput);
    if (ports == NULL) {
        write_log(state->interface, "No physical playback ports\n");
        iface_close(state->interface->id);
        return;
    }

    if (jack_connect(state->client,
            jack_port_name(state->output_port_l), ports[0])) {
        write_log(state->interface, "Cannot connect output ports\n");
    }

    if (jack_connect(state->client,
            jack_port_name(state->output_port_r), ports[1])) {
        write_log(state->interface, "Cannot connect output ports\n");
    }

    /*
     * Calculating the total latency only for one channel and assuming
     * the other channel has exactly the same latency.
     */
    jack_latency_range_t playback_latency;
    jack_port_get_latency_range(
        jack_port_by_name(state->client, ports[0]),
        JackPlaybackLatency,
        &playback_latency);

    jack_free(ports);

    /* We should handle situations where min!=max, but these are rare */
    state->total_latency = capture_latency.min + playback_latency.min;
}

struct Driver jack_driver = {
    .create_state_object = jack_create_state_object,
    .init = jack_iface_init,
    .destroy = jack_destroy,
    .set_position = jack_set_position,
    .set_is_transport_rolling = jack_set_is_transport_rolling,
};
