#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "input_chunk.h"
#include "pa_ringbuffer.h"

struct Interface;

union TaskArgument
{
    void *pointer;
    int integer;
};

struct Message
{
    int type;
    union TaskArgument arg;
};

/* Python thread -> I/O thread */
#define MSG_SET_PLAYSPEC        1  /* arg_ptr is the pointer to Playspec */
#define MSG_UNREF_AUDIO_CLIP   2  /* arg_ptr is the pointer to AudioClip */
#define MSG_SET_POS             3  /* arg_int is the position in frames */
#define MSG_SET_TRANSPORT_STATE 4  /* arg_int is 1 for play, 0 for pause */
/* I/O thread -> Python thread */
#define MSG_DESTROY_AUDIO_CLIP 5  /* arg_ptr is the pointer to AudioClip */
#define MSG_DESTROY_PLAYSPEC    6  /* arg_ptr is the pointer to Playspec */
#define MSG_FRAME_RATE          7  /* arg_int is the frame rate */
#define MSG_CURRENT_POS         8  /* arg_int is the current position in frames */
#define MSG_TRANSPORT_STATE     9  /* arg_int is 1 if rolling, 0 is paused */

/* Ring buffer implementation requires these to be powers of two! */
#define THREAD_QUEUE_SIZE 2048
#define LOG_QUEUE_SIZE 65536
#define INPUT_CLIP_QUEUE_SIZE 2048

bool send_message_with_ptr(
        PaUtilRingBuffer *buffer, int type, void *arg_ptr);
bool send_message_with_int(
        PaUtilRingBuffer *buffer, int type, int arg_int);
bool write_log(struct Interface *state, char *s);
void io_get_logs(struct Interface *interface, char *bytearray, int n);

bool write_input_samples(
    struct Interface *interface, struct InputChunk *input_chunk);
struct InputChunk * io_get_input_chunk(struct Interface *interface);

#endif
