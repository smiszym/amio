%module core

%include <pybuffer.i>

%pybuffer_binary(char *bytes, int n);
%pybuffer_mutable_binary(char *bytearray, int n);

%{

#include <stdbool.h>

struct AudioClip;
struct InputChunk;

/* AudioClip */

struct AudioClip * AudioClip_init(
    char *bytes, int n, int channels, float framerate);
void AudioClip_del(int interface, struct AudioClip *clip);

/* InputChunk */

int InputChunk_get_starting_frame(struct InputChunk *clip);
int InputChunk_get_was_transport_rolling(struct InputChunk *clip);
int InputChunk_get_samples(struct InputChunk *clip, char *bytearray, int n);
void InputChunk_del(struct InputChunk *clip);

/* Playspec */

bool begin_defining_playspec(int size, int insert_at, int start_from);
void set_entry_in_playspec(
    int n,
    struct AudioClip *clip,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);

/* Interface */

void iface_process_messages_on_python_queue(int interface_id);
void iface_get_logs(int interface_id, char *bytearray, int n);
void iface_set_playspec(int interface_id);
int iface_get_frame_rate(int interface_id);
int iface_get_position(int interface_id);
void iface_set_position(int interface_id, int position);
int iface_get_transport_rolling(int interface_id);
void iface_set_transport_rolling(int interface_id, int rolling);
struct InputChunk * iface_get_input_chunk(int interface_id);
void iface_close(int interface_id);

/* drivers */

int create_jack_interface(const char *client_name);

%}

/* AudioClip */

struct AudioClip * AudioClip_init(
    char *bytes, int n, int channels, float framerate);
void AudioClip_del(int interface, struct AudioClip *clip);

/* InputChunk */

int InputChunk_get_starting_frame(struct InputChunk *clip);
int InputChunk_get_was_transport_rolling(struct InputChunk *clip);
int InputChunk_get_samples(struct InputChunk *clip, char *bytearray, int n);
void InputChunk_del(struct InputChunk *clip);

/* Playspec */

bool begin_defining_playspec(int size, int insert_at, int start_from);
void set_entry_in_playspec(
    int n,
    struct AudioClip *clip,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);

/* Interface */

void iface_process_messages_on_python_queue(int interface_id);
void iface_get_logs(int interface_id, char *bytearray, int n);
void iface_set_playspec(int interface_id);
int iface_get_frame_rate(int interface_id);
int iface_get_position(int interface_id);
void iface_set_position(int interface_id, int position);
int iface_get_transport_rolling(int interface_id);
void iface_set_transport_rolling(int interface_id, int rolling);
struct InputChunk * iface_get_input_chunk(int interface_id);
void iface_close(int interface_id);

/* drivers */

int create_jack_interface(const char *client_name);
