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


extern struct popargs args;

static const struct parser_state_transition * client_parser_state_table[] = {
        // client_parser_header_state,
        // client_parser_command_state,
        // client_parser_id_state,
        // client_parser_token_state,
        // client_parser_content_state
};

static const size_t client_parser_state_n[] = {
        // sizeof(client_parser_header_state) / sizeof(client_parser_header_state[0]),
        // sizeof(client_parser_command_state) / sizeof(client_parser_command_state[0]),
        // sizeof(client_parser_id_state) / sizeof(client_parser_id_state[0]),
        // sizeof(client_parser_token_state) / sizeof(client_parser_token_state[0]),
        // sizeof(client_parser_content_state) / sizeof(client_parser_content_state[0]),
};

static const struct parser_definition client_parser_definition = {
        .states = client_parser_state_table,
        .states_count = CLIENT_PARSER_STATES_COUNT,
        .start_state = CLIENT_HEADER,
        .states_n = client_parser_state_n
};

void accept_client_handler(struct selector_key * key) {
    struct sockaddr_in client;
    unsigned int client_size = sizeof(client);

    char read_buffer[MAX_CLIENT_BUFFER] = {0};
    char write_buffer[MAX_CLIENT_BUFFER] = {0};

    ssize_t n = recvfrom(key->fd, read_buffer, MAX_CLIENT_BUFFER, 0, (struct sockaddr *) &client, &client_size);
    if (n <= 0) {
        return;
    }

    struct parser * parser = parser_init(parser_no_classes(), &client_parser_definition);
    // struct client_command command = {0};
}