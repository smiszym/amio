#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#include <jack/jack.h>

#include "communication.h"
#include "driver.h"
#include "playspec.h"

#define MAX_INTERFACES 32

/*
 * Data that is tied to every instantiated interface.
 */
struct Interface
{
    int id;

    /* Information kept on the Python thread to be aware what AudioClips
     * the I/O thread potentially may want to access.
     */
    struct Playspec *py_thread_current_playspec;
    struct Playspec *py_thread_pending_playspec;

    /* Only accessible from the I/O thread */
    struct Playspec *current_playspec;
    struct Playspec *pending_playspec;

    /* Only accessible from the Python thread */
    int last_reported_frame_rate;
    bool last_reported_is_transport_rolling;
    int last_reported_position;

    /*
     * Python thread queue - used for passing messages
     * from the I/O thread to the Python thread
     */
    PaUtilRingBuffer python_thread_queue;
    struct Task *python_thread_queue_buffer;

    /*
     * I/O thread queue - used for passing messages
     * from the Python thread to the I/O thread
     */
    PaUtilRingBuffer io_thread_queue;
    struct Task *io_thread_queue_buffer;

    /*
     * Ring buffer containing characters that were scheduled by the I/O thread
     * to be printed on the Python thread.
     */
    PaUtilRingBuffer log_queue;
    char *log_queue_buffer;

    /*
     * Ring buffer containing input samples that were received by the JACK
     * thread from the audio interface. Read by the Python thread.
     */
    PaUtilRingBuffer input_chunk_queue;
    struct InputChunk *input_chunk_queue_buffer;

    /* Driver talks to audio system such as ALSA, PulseAudio, JACK */
    struct Driver *driver;
    void *driver_state;
};

struct Interface * get_interface_by_id(int id);

int iface_get_key(int interface_id);

void for_each_interface(void (*callback)(int interface_id));

int create_interface(struct Driver *driver, const char *client_name);

int create_jack_interface(const char *client_name);

void iface_close(int interface_id);

#define PY_QUEUE_PROCESSING_RESULT_NOTHING 0
#define PY_QUEUE_PROCESSING_RESULT_PLAYSPEC_APPLIED 1
int iface_process_messages_on_python_queue(int interface_id);

void process_input_with_buffers(
    struct Interface *interface,
    jack_nframes_t nframes,
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r,
    int starting_frame,
    int transport_state);

jack_nframes_t process_output_with_buffers(
    struct Interface *state,
    int frame_in_playspec,
    bool is_transport_rolling,
    jack_nframes_t nframes,
    jack_default_audio_sample_t *port_l,
    jack_default_audio_sample_t *port_r);

int iface_set_playspec(int interface_id);
int iface_get_frame_rate(int interface_id);
int iface_get_position(int interface_id);
void iface_set_position(int interface_id, int position);
int iface_get_transport_rolling(int interface_id);
void iface_set_transport_rolling(int interface_id, int rolling);
int iface_get_current_playspec_id(int interface_id);

#endif
