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
#include "../logger/logger.h"
extern struct popargs args;

static void parse_client(udp_client * connection, char * buffer, size_t n) {

    

}

void accept_client_handler(struct selector_key * key) {
    struct sockaddr_in client;
    unsigned int client_size = sizeof(client);

    char read_buffer[MAX_CLIENT_BUFFER] = {0};
    // char write_buffer[MAX_CLIENT_BUFFER] = {0};

    ssize_t n = recvfrom(key->fd, read_buffer, MAX_CLIENT_BUFFER, 0, (struct sockaddr *) &client, &client_size);
    if (n <= 0) {
        return;
    }

    log(INFO, "UDP Client - Recived Message: %s", read_buffer);
    udp_client * connection = calloc(1, sizeof(udp_client));
    connection->command[0] = 0;
    connection->command_idx = 0;
    connection->arguments[0] = 0;
    connection->args_idx = 0;
    parse_client(connection,read_buffer, n);

}