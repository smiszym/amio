%module core

%include <pybuffer.i>

%pybuffer_binary(char *bytes, int n);
%pybuffer_mutable_binary(char *bytearray, int n);

%{
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

void Playspec_init(int size);
void Playspec_setEntry(
    int n,
    struct AudioClip *clip,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);
void Playspec_setInsertionPoints(int insert_at, int start_from);

/* JackInterface interface */

void jackio_process_messages_on_python_queue(
    struct JackInterface *jack_interface);
struct JackInterface * jackio_init(const char *client_name);
void jackio_get_logs(struct JackInterface *jack_interface, char *bytearray, int n);
void jackio_set_playspec(struct JackInterface *interface);
int jackio_get_frame_rate(struct JackInterface *interface);
int jackio_get_position(struct JackInterface *interface);
void jackio_set_position(struct JackInterface *interface, int position);
int jackio_get_transport_rolling(struct JackInterface *interface);
void jackio_set_transport_rolling(struct JackInterface *interface, int rolling);
struct InputChunk * jackio_get_input_chunk(struct JackInterface *jack_interface);
void jackio_close(struct JackInterface *jack_interface);
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

void Playspec_init(int size);
void Playspec_setEntry(
    int n,
    struct AudioClip *clip,
    int clip_frame_a, int clip_frame_b,
    int play_at_frame, int repeat_interval,
    float gain_l, float gain_r);
void Playspec_setInsertionPoints(int insert_at, int start_from);

/* JackInterface interface */

void jackio_process_messages_on_python_queue(
    struct JackInterface *jack_interface);
struct JackInterface * jackio_init(const char *client_name);
void jackio_get_logs(struct JackInterface *jack_interface, char *bytearray, int n);
void jackio_set_playspec(struct JackInterface *interface);
int jackio_get_frame_rate(struct JackInterface *interface);
int jackio_get_position(struct JackInterface *interface);
void jackio_set_position(struct JackInterface *interface, int position);
int jackio_get_transport_rolling(struct JackInterface *interface);
void jackio_set_transport_rolling(struct JackInterface *interface, int rolling);
struct InputChunk * jackio_get_input_chunk(struct JackInterface *jack_interface);
void jackio_close(struct JackInterface *jack_interface);
