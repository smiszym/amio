#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver.h"
#include "input_chunk.h"
#include "pa_ringbuffer.h"

struct Interface;

union TaskArgument
{
    void *pointer;
    int integer;
};

typedef void (*IoThreadCallable)(
    struct Interface *state,
    struct DriverInterface *driver,
    void *driver_handle,
    union TaskArgument arg);

typedef void (*PyThreadCallable)(
    struct Interface *interface,
    union TaskArgument arg);

union TaskCallable
{
    PyThreadCallable py_thread_callable;
    IoThreadCallable io_thread_callable;
};

struct Message
{
    union TaskCallable callable;
    union TaskArgument arg;
};

/* Ring buffer implementation requires these to be powers of two! */
#define THREAD_QUEUE_SIZE 2048
#define LOG_QUEUE_SIZE 65536
#define INPUT_CLIP_QUEUE_SIZE 2048

bool send_message_with_ptr_to_py_thread(
    struct Interface *interface, PyThreadCallable callable, void *arg_ptr);
bool send_message_with_ptr_to_io_thread(
    struct Interface *interface, IoThreadCallable callable, void *arg_ptr);
bool send_message_with_int_to_py_thread(
    struct Interface *interface, PyThreadCallable callable, int arg_int);
bool send_message_with_int_to_io_thread(
    struct Interface *interface, IoThreadCallable callable, int arg_int);
bool write_log(struct Interface *state, char *s);
void io_get_logs(struct Interface *interface, char *bytearray, int n);

bool write_input_samples(
    struct Interface *interface, struct InputChunk *input_chunk);
struct InputChunk * io_get_input_chunk(struct Interface *interface);

#endif
