#include "communication.h"

#include <stdlib.h>

#include "interface.h"
#include "string.h"

bool post_task_with_ptr_to_py_thread(
        struct Interface *interface, PyThreadCallable callable, void *arg_ptr) {
    struct Task msg;
    msg.callable.py_thread_callable = callable;
    msg.arg.pointer = arg_ptr;
    return PaUtil_WriteRingBuffer(&interface->python_thread_queue, &msg, 1) > 0;
}

bool post_task_with_ptr_to_io_thread(
        struct Interface *interface, IoThreadCallable callable, void *arg_ptr) {
    struct Task msg;
    msg.callable.io_thread_callable = callable;
    msg.arg.pointer = arg_ptr;
    return PaUtil_WriteRingBuffer(&interface->io_thread_queue, &msg, 1) > 0;
}

bool post_task_with_int_to_py_thread(
        struct Interface *interface, PyThreadCallable callable, int arg_int) {
    struct Task msg;
    msg.callable.py_thread_callable = callable;
    msg.arg.integer = arg_int;
    return PaUtil_WriteRingBuffer(&interface->python_thread_queue, &msg, 1) > 0;
}

bool post_task_with_int_to_io_thread(
        struct Interface *interface, IoThreadCallable callable, int arg_int) {
    struct Task msg;
    msg.callable.io_thread_callable = callable;
    msg.arg.integer = arg_int;
    return PaUtil_WriteRingBuffer(&interface->io_thread_queue, &msg, 1) > 0;
}

bool write_log(struct Interface *state, char *s)
{
    int len = strlen(s);
    return PaUtil_WriteRingBuffer(&state->log_queue, s, len) == len;
}

void iface_get_logs(int interface_id, char *bytearray, int n)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);

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

bool iface_begin_reading_input_chunk(int interface_id)
{
    /* Runs on the Python thread */

    struct Interface *interface = get_interface_by_id(interface_id);

    if (PaUtil_ReadRingBuffer(
            &interface->input_chunk_queue, &input_chunk_being_read, 1) == 1) {
        return 1;
    } else {
        return 0;
    }
}
