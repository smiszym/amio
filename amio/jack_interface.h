#ifndef JACK_CLIENT_H
#define JACK_CLIENT_H

struct Interface;

struct Interface * create_jack_interface(const char *client_name);

#endif
