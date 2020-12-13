#ifndef DRIVER_H
#define DRIVER_H

struct Interface;

struct Driver
{
    void * (*create_state_object)(
        const char *client_name, struct Interface *interface);
    void (*init)(void *driver_state);
    void (*destroy)(void *driver_state);
    void (*set_position)(void *driver_state, int position);
    void (*set_is_transport_rolling)(void *driver_state, bool value);
};

#endif