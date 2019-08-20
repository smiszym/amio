#include "communication.h"

#include <stdlib.h>

#include "interface.h"
#include "string.h"

bool send_message_with_ptr(
        PaUtilRingBuffer *buffer, int type, void *arg_ptr) {
    struct Message msg;
    msg.type = type;
    msg.arg_ptr = arg_ptr;
    return PaUtil_WriteRingBuffer(buffer, &msg, 1) > 0;
}

bool send_message_with_int(
        PaUtilRingBuffer *buffer, int type, int arg_int) {
    struct Message msg;
    msg.type = type;
    msg.arg_int = arg_int;
    return PaUtil_WriteRingBuffer(buffer, &msg, 1) > 0;
}

bool write_log(struct Interface *state, char *s)
{
    int len = strlen(s);
    return PaUtil_WriteRingBuffer(&state->log_queue, s, len) == len;
}

void io_get_logs(struct Interface *interface, char *bytearray, int n)
{
    /* Runs on the Python thread */

    char buf[LOG_QUEUE_SIZE];
    int to_read = LOG_QUEUE_SIZE < (n-1) ? LOG_QUEUE_SIZE : (n-1);
    int actually_read;

    actually_read = PaUtil_ReadRingBuffer(&interface->log_queue, buf, to_read);
    memcpy(bytearray, buf, actually_read);
    bytearray[actually_read] = '\0';
}

bool write_input_samples(
    struct Interface *interface, struct InputChunk *input_chunk)
{
    return PaUtil_WriteRingBuffer(
        &interface->input_chunk_queue, input_chunk, 1) == 1;
}

struct InputChunk * io_get_input_chunk(struct Interface *interface)
{
    /* Runs on the Python thread */

    struct InputChunk *clip = malloc(sizeof(struct InputChunk));
    if (PaUtil_ReadRingBuffer(&interface->input_chunk_queue, clip, 1) == 1) {
        return clip;
    } else {
        free(clip);
        return NULL;
    }
}
