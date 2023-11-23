#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdlib.h>

#include "../selector/selector.h"
#define MAX_CLIENT_BUFFER 8192

typedef enum {
    CLIENT_HEADER = 0,
    CLIENT_COMMAND,
    CLIENT_ID,
    CLIENT_TOKEN,
    CLIENT_CONTENT,
    CLIENT_PARSER_STATES_COUNT
} client_parser_states;

typedef enum client_commands{
    U,
    C,
    R,
    D,
    M,
    S,
    L
}client_commands;


void accept_client_handler(struct selector_key * key);

#endif