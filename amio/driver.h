#ifndef DRIVER_H
#define DRIVER_H

struct Interface;

struct Driver
{
    void * (*create_state_object)(
        const char *client_name, struct Interface *interface);
    void (*init)(void *state);
    void (*destroy)(void *state);
    void (*set_position)(void *handle, int position);
    void (*set_is_transport_rolling)(void *handle, bool value);
};

#endif