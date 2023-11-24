#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdlib.h>

#include "../selector/selector.h"
#define MAX_CLIENT_BUFFER 8192
#define CLIENT_ID_LENGTH 11
#define CLIENT_TOKEN_LENGTH 7
#define CLIENT_CONTENT_LENGTH 255

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