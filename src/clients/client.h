#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdlib.h>

#include "../selector/selector.h"
#define MAX_BUFFER 256
#define MAX_CLIENT_BUFFER 4096

typedef enum client_commands{
    U,
    C,
    R,
    D,
    M,
    S,
    L
}client_commands;

typedef struct udp_client
{
    char command[MAX_BUFFER];
    size_t command_idx;
    char arguments[MAX_BUFFER];
    size_t args_idx;
} udp_client;


void accept_client_handler(struct selector_key * key);

#endif