#include <stdio.h>
#include <stdlib.h>  // malloc
#include <string.h>  // memset
#include <assert.h>  // assert
#include <errno.h>
#include <time.h>
#include <unistd.h>  // close
#include <pthread.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "client.h"
#include "client_utils.h"
#include "../parser/parser.h"

extern struct popargs args;

void accept_client_handler(struct selector_key * key) {
    struct sockaddr_in client;
    unsigned int client_size = sizeof(client);

    char read_buffer[MAX_CLIENT_BUFFER] = {0};
    // char write_buffer[MAX_CLIENT_BUFFER] = {0};

    ssize_t n = recvfrom(key->fd, read_buffer, MAX_CLIENT_BUFFER, 0, (struct sockaddr *) &client, &client_size);
    if (n <= 0) {
        return;
    }

}