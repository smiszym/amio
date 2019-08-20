#ifndef DRIVER_H
#define DRIVER_H

struct DriverInterface
{
    void (*set_position)(void *handle, int position);
    void (*set_is_transport_rolling)(void *handle, bool value);
};

#endif