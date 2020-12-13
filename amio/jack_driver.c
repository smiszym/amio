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

static void jack_iface_init(void *state);

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

static void jack_destroy(void *state)
{
    struct JackDriverState *jack_interface = state;
    jack_client_close(jack_interface->client);
    free(jack_interface);
}

static void jack_set_position(void *handle, int position)
{
    /* Runs on the I/O thread */
    struct JackDriverState *client = handle;
    client->frame_in_playspec = position;
}

static void jack_set_is_transport_rolling(void *handle, bool value)
{
    /* Runs on the I/O thread */
    struct JackDriverState *client = handle;
    client->is_transport_rolling = value;
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

    int new_frame = process_input_output_with_buffers(
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
    // TODO instead of quitting, handle all JACK failures
    exit(1);
}

static void jack_iface_init(void *state)
{
    /* Runs on the Python thread */

    struct JackDriverState *jack_interface = state;

    const char **ports;
    jack_options_t options = JackNullOption;
    jack_status_t status;

    jack_interface->is_transport_rolling = false;
    jack_interface->frame_in_playspec = 0;

    jack_interface->client = jack_client_open(
        jack_interface->client_name, options, &status, NULL);
    if (jack_interface->client == NULL) {
        fprintf(stderr, "jack_client_open() failed, "
            "status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            fprintf(stderr, "Unable to connect to JACK server\n");
        }
        exit(1);
    }
    if (status & JackServerStarted) {
        fprintf(stderr, "JACK server started\n");
    }
    if (status & JackNameNotUnique) {
        jack_interface->client_name =
            jack_get_client_name(jack_interface->client);
        fprintf(stderr, "Unique name `%s' assigned\n",
            jack_interface->client_name);
    }

    post_task_with_int_to_py_thread(
        jack_interface->interface, py_thread_receive_frame_rate,
        jack_get_sample_rate(jack_interface->client));

    jack_set_process_callback(
        jack_interface->client, process, jack_interface);
    jack_on_shutdown(jack_interface->client, jack_shutdown, 0);

    jack_interface->input_port_l = jack_port_register(
        jack_interface->client, "input_l",
        JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsInput, 0);

    jack_interface->input_port_r = jack_port_register(
        jack_interface->client, "input_r",
        JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsInput, 0);

    if ((jack_interface->input_port_l == NULL)
            || (jack_interface->input_port_r == NULL)) {
        fprintf(stderr, "No more JACK ports available\n");
        exit(1);
    }

    jack_interface->output_port_l = jack_port_register(
        jack_interface->client, "output_l",
        JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsOutput, 0);

    jack_interface->output_port_r = jack_port_register(
        jack_interface->client, "output_r",
        JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsOutput, 0);

    if ((jack_interface->output_port_l == NULL)
            || (jack_interface->output_port_r == NULL)) {
        fprintf(stderr, "No more JACK ports available\n");
        exit(1);
    }

    if (jack_activate(jack_interface->client)) {
        fprintf(stderr, "Cannot activate JACK client");
        exit(1);
    }

    ports = jack_get_ports(jack_interface->client, NULL, NULL,
                           JackPortIsPhysical|JackPortIsOutput);
    if (ports == NULL) {
        fprintf(stderr, "No physical capture ports\n");
        exit(1);
    }

    if (jack_connect(jack_interface->client,
            ports[0], jack_port_name(jack_interface->input_port_l))) {
        fprintf(stderr, "Cannot connect input ports\n");
    }

    if (jack_connect(jack_interface->client,
            ports[1], jack_port_name(jack_interface->input_port_r))) {
        fprintf(stderr, "Cannot connect input ports\n");
    }

    /*
     * Calculating the total latency only for one channel and assuming
     * the other channel has exactly the same latency.
     */
    jack_latency_range_t capture_latency;
    jack_port_get_latency_range(
        jack_port_by_name(jack_interface->client, ports[0]),
        JackCaptureLatency,
        &capture_latency);

    jack_free(ports);

    ports = jack_get_ports(jack_interface->client, NULL, NULL,
                           JackPortIsPhysical|JackPortIsInput);
    if (ports == NULL) {
        fprintf(stderr, "No physical playback ports\n");
        exit(1);
    }

    if (jack_connect(jack_interface->client,
            jack_port_name(jack_interface->output_port_l), ports[0])) {
        fprintf(stderr, "Cannot connect output ports\n");
    }

    if (jack_connect(jack_interface->client,
            jack_port_name(jack_interface->output_port_r), ports[1])) {
        fprintf(stderr, "Cannot connect output ports\n");
    }

    /*
     * Calculating the total latency only for one channel and assuming
     * the other channel has exactly the same latency.
     */
    jack_latency_range_t playback_latency;
    jack_port_get_latency_range(
        jack_port_by_name(jack_interface->client, ports[0]),
        JackPlaybackLatency,
        &playback_latency);

    jack_free(ports);

    /* We should handle situations where min!=max, but these are rare */
    jack_interface->total_latency = capture_latency.min + playback_latency.min;
}

struct Driver jack_driver = {
    .create_state_object = jack_create_state_object,
    .init = jack_iface_init,
    .destroy = jack_destroy,
    .set_position = jack_set_position,
    .set_is_transport_rolling = jack_set_is_transport_rolling,
};
