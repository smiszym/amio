%module core

%include <pybuffer.i>

%pybuffer_binary(char *bytes, int n);
%pybuffer_mutable_binary(char *bytearray, int n);

%{
#include <stdbool.h>

struct AudioClip;
struct InputChunk;
struct JackInterface;

/* AudioClip interface */

struct AudioClip * AudioClip_init(
    char *bytes, int n, int channels, float framerate);
void AudioClip_del(
    struct JackInterface *jack_interface, struct AudioClip *clip);

/* InputChunk interface */

int InputChunk_get_starting_frame(struct InputChunk *clip);
int InputChunk_get_was_transport_rolling(struct InputChunk *clip);
int InputChunk_get_samples(struct InputChunk *clip, char *bytearray, int n);
void InputChunk_del(struct InputChunk *clip);

/* Playspec interface */

bool begin_defining_playspec(int size, int insert_at, int start_from);
void set_entry_in_playspec(
    int n,
    struct AudioClip *clip,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);

/* JackInterface interface */

void jack_iface_process_messages_on_python_queue(
    struct JackInterface *jack_interface);
struct JackInterface * jack_iface_init(const char *client_name);
void jack_iface_get_logs(struct JackInterface *jack_interface, char *bytearray, int n);
void jack_iface_set_playspec(struct JackInterface *interface);
int jack_iface_get_frame_rate(struct JackInterface *interface);
int jack_iface_get_position(struct JackInterface *interface);
void jack_iface_set_position(struct JackInterface *interface, int position);
int jack_iface_get_transport_rolling(struct JackInterface *interface);
void jack_iface_set_transport_rolling(struct JackInterface *interface, int rolling);
struct InputChunk * jack_iface_get_input_chunk(struct JackInterface *jack_interface);
void jack_iface_close(struct JackInterface *jack_interface);
%}

/* AudioClip interface */

struct AudioClip * AudioClip_init(
    char *bytes, int n, int channels, float framerate);
void AudioClip_del(
    struct JackInterface *jack_interface, struct AudioClip *clip);

/* InputChunk interface */

int InputChunk_get_starting_frame(struct InputChunk *clip);
int InputChunk_get_was_transport_rolling(struct InputChunk *clip);
int InputChunk_get_samples(struct InputChunk *clip, char *bytearray, int n);
void InputChunk_del(struct InputChunk *clip);

/* Playspec interface */

bool begin_defining_playspec(int size, int insert_at, int start_from);
void set_entry_in_playspec(
    int n,
    struct AudioClip *clip,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);

/* JackInterface interface */

void jack_iface_process_messages_on_python_queue(
    struct JackInterface *jack_interface);
struct JackInterface * jack_iface_init(const char *client_name);
void jack_iface_get_logs(struct JackInterface *jack_interface, char *bytearray, int n);
void jack_iface_set_playspec(struct JackInterface *interface);
int jack_iface_get_frame_rate(struct JackInterface *interface);
int jack_iface_get_position(struct JackInterface *interface);
void jack_iface_set_position(struct JackInterface *interface, int position);
int jack_iface_get_transport_rolling(struct JackInterface *interface);
void jack_iface_set_transport_rolling(struct JackInterface *interface, int rolling);
struct InputChunk * jack_iface_get_input_chunk(struct JackInterface *jack_interface);
void jack_iface_close(struct JackInterface *jack_interface);
