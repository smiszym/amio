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
    struct Driver *driver,
    void *driver_handle,
    union TaskArgument arg);

typedef int (*PyThreadCallable)(
    struct Interface *interface,
    union TaskArgument arg);

union TaskCallable
{
    PyThreadCallable py_thread_callable;
    IoThreadCallable io_thread_callable;
};

struct Task
{
    union TaskCallable callable;
    union TaskArgument arg;
};

/* Ring buffer implementation requires these to be powers of two! */
#define THREAD_QUEUE_SIZE 2048
#define LOG_QUEUE_SIZE 65536
#define INPUT_CLIP_QUEUE_SIZE 2048

bool post_task_with_ptr_to_py_thread(
    struct Interface *interface, PyThreadCallable callable, void *arg_ptr);
bool post_task_with_ptr_to_io_thread(
    struct Interface *interface, IoThreadCallable callable, void *arg_ptr);
bool post_task_with_int_to_py_thread(
    struct Interface *interface, PyThreadCallable callable, int arg_int);
bool post_task_with_int_to_io_thread(
    struct Interface *interface, IoThreadCallable callable, int arg_int);
bool write_log(struct Interface *state, char *s);
void iface_get_logs(int interface_id, char *bytearray, int n);

bool write_input_samples(
    struct Interface *interface, struct InputChunk *input_chunk);
bool iface_begin_reading_input_chunk(int interface_id);

#endif
