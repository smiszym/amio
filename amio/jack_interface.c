#include "jack_interface.h"

#include <sndfile.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio_clip.h"
#include "interface.h"
#include "communication.h"
#include "mixer.h"
#include "playspec.h"

static void jack_set_position(void *handle, int position)
{
    /* Runs on the I/O thread */
    struct JackInterface *client = handle;
    client->frame_in_playspec = position;
}

static void jack_set_is_transport_rolling(void *handle, bool value)
{
    /* Runs on the I/O thread */
    struct JackInterface *client = handle;
    client->is_transport_rolling = value;
}

static struct DriverInterface jack_driver = {
    .set_position = jack_set_position,
    .set_is_transport_rolling = jack_set_is_transport_rolling,
};

static int process(jack_nframes_t nframes, void *arg)
{
    /* Runs on the I/O thread */

    struct JackInterface *amio_jack_client = arg;

    jack_default_audio_sample_t *in_buffer_l, *in_buffer_r;
    jack_default_audio_sample_t *out_buffer_l, *out_buffer_r;

    in_buffer_l = (jack_default_audio_sample_t*)jack_port_get_buffer(
        amio_jack_client->input_port_l, nframes);
    in_buffer_r = (jack_default_audio_sample_t*)jack_port_get_buffer(
        amio_jack_client->input_port_r, nframes);

    process_input_with_buffers(
        &amio_jack_client->interface,
        nframes,
        in_buffer_l,
        in_buffer_r,
        amio_jack_client->frame_in_playspec - amio_jack_client->total_latency,
        amio_jack_client->is_transport_rolling);

    out_buffer_l = (jack_default_audio_sample_t*)jack_port_get_buffer(
        amio_jack_client->output_port_l, nframes);
    out_buffer_r = (jack_default_audio_sample_t*)jack_port_get_buffer(
        amio_jack_client->output_port_r, nframes);

    int old_frame = amio_jack_client->frame_in_playspec;

    int new_frame = process_input_output_with_buffers(
        &amio_jack_client->interface,
        &jack_driver,
        amio_jack_client,
        amio_jack_client->frame_in_playspec,
        amio_jack_client->is_transport_rolling,
        nframes, out_buffer_l, out_buffer_r);

    /* Only advance the current position if it wasn't changed from Python. */
    if (amio_jack_client->frame_in_playspec == old_frame)
        amio_jack_client->frame_in_playspec = new_frame;

    return 0;
}

static void jack_shutdown(void *arg)
{
    // TODO instead of quitting, handle all JACK failures
    exit(1);
}

void jackio_process_messages_on_python_queue(
    struct JackInterface *jack_interface)
{
    io_process_messages_on_python_queue(&jack_interface->interface);
}

struct JackInterface * jackio_init(const char *client_name)
{
    /* Runs on the Python thread */

    struct JackInterface *jack_interface = malloc(sizeof(
                                                     struct JackInterface));

    io_init(&jack_interface->interface);

    const char **ports;
    jack_options_t options = JackNullOption;
    jack_status_t status;

    jack_interface->is_transport_rolling = false;
    jack_interface->frame_in_playspec = 0;

    jack_interface->client = jack_client_open(
        client_name, options, &status, NULL);
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
        client_name = jack_get_client_name(jack_interface->client);
        fprintf(stderr, "Unique name `%s' assigned\n", client_name);
    }

    send_message_with_int(
        &jack_interface->interface.python_thread_queue, MSG_FRAME_RATE,
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

    return jack_interface;
}

void jackio_close(struct JackInterface *jack_interface)
{
    /* Runs on the Python thread */

    jack_client_close(jack_interface->client);
    io_close(&jack_interface->interface);
    free(jack_interface);
}

void jackio_get_logs(
    struct JackInterface *jack_interface, char *bytearray, int n)
{
    io_get_logs(&jack_interface->interface, bytearray, n);
}

void jackio_set_playspec(struct JackInterface *jack_interface,
                  struct Playspec *playspec)
{
    io_set_playspec(&jack_interface->interface, playspec);
}

int jackio_get_frame_rate(struct JackInterface *jack_interface)
{
    return io_get_frame_rate(&jack_interface->interface);
}

int jackio_get_position(struct JackInterface *jack_interface)
{
    return io_get_position(&jack_interface->interface);
}

void jackio_set_position(struct JackInterface *jack_interface, int position)
{
    io_set_position(&jack_interface->interface, position);
}

int jackio_get_transport_rolling(struct JackInterface *jack_interface)
{
    return io_get_transport_rolling(&jack_interface->interface);
}

void jackio_set_transport_rolling(struct JackInterface *jack_interface, int rolling)
{
    io_set_transport_rolling(&jack_interface->interface, rolling);
}

struct InputChunk * jackio_get_input_chunk(struct JackInterface *jack_interface)
{
    return io_get_input_chunk(&jack_interface->interface);
}
