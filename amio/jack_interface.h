#ifndef JACK_CLIENT_H
#define JACK_CLIENT_H

#include <jack/jack.h>

#include "interface.h"
#include "communication.h"

/*
 * The design distinguishes two threads for each client:
 *   * the Python thread (the main thread on which Python executes)
 *   * the I/O thread (the thread on which the process callback executes)
 */

struct JackInterface
{
    struct Interface interface;

    jack_client_t *client;
    jack_port_t *input_port_l;
    jack_port_t *input_port_r;
    jack_port_t *output_port_l;
    jack_port_t *output_port_r;

    int total_latency;

    bool is_transport_rolling;
    int frame_in_playspec;  /* position in the whole playspec */
};

void jackio_process_messages_on_python_queue(
    struct JackInterface *jack_interface);

struct JackInterface * jackio_init(const char *client_name);
void jackio_close(struct JackInterface *jack_interface);

void jackio_get_logs(
    struct JackInterface *jack_interface, char *bytearray, int n);

void jackio_set_playspec(struct JackInterface *jack_interface);

int jackio_get_frame_rate(struct JackInterface *jack_interface);

int jackio_get_position(struct JackInterface *jack_interface);

void jackio_set_position(struct JackInterface *jack_interface, int position);

int jackio_get_transport_rolling(struct JackInterface *jack_interface);

void jackio_set_transport_rolling(struct JackInterface *jack_interface, int rolling);

struct InputChunk * jackio_get_input_chunk(struct JackInterface *jack_interface);

#endif
