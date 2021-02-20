%module native

%include <pybuffer.i>

%pybuffer_binary(char *bytes, int n);
%pybuffer_mutable_binary(char *bytearray, int n);

%{

#include <stdbool.h>

/* AudioClip */

int AudioClip_init(char *bytes, int n, int channels, float framerate);
void AudioClip_del(int interface, int clip_id);

/* InputChunk */

int InputChunk_get_playspec_id();
int InputChunk_get_starting_frame();
int InputChunk_get_was_transport_rolling();
int InputChunk_get_samples(char *bytearray, int n);

/* Playspec */

bool begin_defining_playspec(int size, int insert_at, int start_from);
void set_entry_in_playspec(
    int n,
    int clip_id,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);

/* Interface */

int iface_process_messages_on_python_queue(int interface_id);
void iface_get_logs(int interface_id, char *bytearray, int n);
int iface_set_playspec(int interface_id);
int iface_get_frame_rate(int interface_id);
int iface_get_position(int interface_id);
void iface_set_position(int interface_id, int position);
int iface_get_transport_rolling(int interface_id);
void iface_set_transport_rolling(int interface_id, int rolling);
int iface_get_current_playspec_id(int interface_id);
bool iface_begin_reading_input_chunk(int interface_id);
void iface_close(int interface_id);

/* drivers */

int create_jack_interface(const char *client_name);

%}

/* AudioClip */

int AudioClip_init(char *bytes, int n, int channels, float framerate);
void AudioClip_del(int interface, int clip_id);

/* InputChunk */

int InputChunk_get_playspec_id();
int InputChunk_get_starting_frame();
int InputChunk_get_was_transport_rolling();
int InputChunk_get_samples(char *bytearray, int n);

/* Playspec */

bool begin_defining_playspec(int size, int insert_at, int start_from);
void set_entry_in_playspec(
    int n,
    int clip_id,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);

/* Interface */

int iface_process_messages_on_python_queue(int interface_id);
void iface_get_logs(int interface_id, char *bytearray, int n);
int iface_set_playspec(int interface_id);
int iface_get_frame_rate(int interface_id);
int iface_get_position(int interface_id);
void iface_set_position(int interface_id, int position);
int iface_get_transport_rolling(int interface_id);
void iface_set_transport_rolling(int interface_id, int rolling);
int iface_get_current_playspec_id(int interface_id);
bool iface_begin_reading_input_chunk(int interface_id);
void iface_close(int interface_id);

/* drivers */

int create_jack_interface(const char *client_name);
