/**
 * pop3.c  - 
 */
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

// #include "hello.h"
// #include "request.h"
#include "../buffer/buffer.h"
#include "pop3.h"
#include "../net/netutils.h"
#include "pop3_stm_handlers.h"
#include "pop_utils.h"

struct state_definition stm_states_table[] = {
        {
                .state = AUTHORIZATION,
                .on_arrival = authorization_arrival,
                .on_departure = authorization_departure,
                .on_read_ready = authorization_read,
                .on_write_ready = authorization_write
        },
        {
                .state = TRANSACTION,
                .on_arrival = transaction_arrival,
                .on_departure = transaction_departure,
                .on_read_ready = transaction_read,
                .on_write_ready = transaction_write
        },
        {
                .state = ERROR,
                .on_arrival = error_arrival,
                .on_departure = error_departure,
                .on_read_ready = error_read,
                .on_write_ready = error_write
        },
        {
                .state = QUIT,
                .on_arrival = quit_arrival,
                .on_departure = quit_departure,
                .on_read_ready = quit_read,
                .on_write_ready = quit_write
        }
};

static const struct parser_state_transition parser_command[] = {
        {.when = ' ', .dest = ARGUMENT, .act1 = parser_command_space},
        {.when = '\r', .dest = END, .act1 = parser_command_return},
        {.when = ANY, .dest = COMMAND, .act1 = parser_command_any}
};

static const struct parser_state_transition parser_argument[] = {
        {.when = '\r', .dest = END, .act1 = parser_argument_return},
        {.when = ANY, .dest = ARGUMENT, .act1 = parser_argument_any}
};

static const struct parser_state_transition parser_end[] = {
        {.when = '\n', .dest = COMMAND, .act1 = parser_end_enter},
        {.when = ANY, .dest = COMMAND, .act1 = parser_end_any}
};

static const struct parser_state_transition * parser_table[] = {
        parser_command,
        parser_argument,
        parser_end
};

static const size_t states_dim[] = {
        sizeof(parser_command) / sizeof(parser_command[0]),
        sizeof(parser_argument) / sizeof(parser_argument[0]),
        sizeof(parser_end) / sizeof(parser_end[0]),
};

struct parser_definition parser_deff = {
    .states = parser_table,
    .states_count = PARSER_STATES_COUNT,
    .start_state = COMMAND,
    .states_n = states_dim
};



// Ejemplo de fd_handler para el cliente
static const struct fd_handler client_handler = {
    .handle_read = client_read,   // Función que maneja la lectura.
    .handle_write = client_write,         // Si no necesitas manejar escritura inmediatamente.
    .handle_close = client_close, // Función que maneja el cierre.
    // .handle_block = NULL,       // Si tienes un handler para operaciones bloqueantes.
};

static int get_idx_of_connection(int socket, connection * clients){
    for (int i = 0 ; i < MAX_CLIENTS ; i++){
        if (clients[i].socket == socket && clients[i].active){
            return i;
        }
    }
    return -1;

}

void client_close(struct selector_key *key) {
}

void client_read(struct selector_key *key) {
    stm_handler_read(&((connection *)key->data)->stm, key);
}

void client_write(struct selector_key *key) {
    stm_handler_write(&((connection *)key->data)->stm, key);
}

void accept_connection_handler(struct selector_key *key) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    const int client_fd = accept(key->fd, (struct sockaddr *)&client_addr, &client_addr_len);
    
    connection * client = calloc(1, sizeof(connection));

    if (client_fd == -1) {
        goto fail;
    }

    // Convierte en non blocking el socket nuevo.
    if (selector_fd_set_nio(client_fd) == -1) {
        goto fail;
    }

    // // Almacenar la conexión (suponiendo que esta función existe y funciona correctamente).
    // int new_idx = store_connection(client, (connection *)key->data);
    // if ( new_idx < 0) {
    //     goto fail;
    // }

    log(INFO,"%s", "Initialized selector, will initialize STM.\n" )
    
    client->stm.states = stm_states_table;
    client->stm.initial = AUTHORIZATION;
    client->stm.max_state = STM_STATES_COUNT;
    stm_init(&client->stm);
    buffer_init(&client->command_buffer, MAX_BUFF_SIZE, (uint8_t *) client->command_buff);
    buffer_init(&client->server_buffer, MAX_BUFF_SIZE, (uint8_t *) client->server_buff);
    client->parser = parser_init(parser_no_classes(), &parser_deff);
    
    // connection->last_state = -1;
    // connection->current_command.mail_fd = -1;
    // buffer_init(&connection->current_command.mail_buffer_object, BUFFER_SIZE, (uint8_t *) connection->current_command.mail_buffer);
    // connection->current_session.mails = calloc(args.max_mails, sizeof(struct mail));
    // connection->current_session.requested_quit = false;

    // Registrar el nuevo socket cliente con el selector.
    if (selector_register(key->s, client_fd, &client_handler, OP_READ, client) != SELECTOR_SUCCESS) {
        goto fail;
    }

    log(DEBUG,"%s","Established new POP3 connection");
    return;

fail:
    log(LOG_ERROR,"%s","FAIL! Could not accept connection\n");
    if (client_fd != -1) {
        free(client);
        close(client_fd);
        log(LOG_ERROR,"%s","FREED resources\n");

    }
}

void user_write_handler(struct selector_key * key){
    write(STDOUT_FILENO, "write handler", 14);
    printf("Esto es el fd de la key: %d \t Esto es su selector \n", key->fd);
    // sendto(key->fd, to_print, bytes_to_send, 0, (struct sockaddr *) &client_addr, client_addr_len);
}

void list(user_state * user) {
    if (!user->auth)
    {
        send(user->socket_fd, "ERROR\n", 7, 0);
        return;
    }else{
        for (int i = 0 ; i < user->inbox_size ; i++){

        }
        // open dir (maildir)
        send(user->socket_fd, "OK! \n",5,0);
    }
    
 }

int store_connection(int socket_fd, connection * clients){
    int idx = get_user_buffer_idx(clients);
    if (idx == -1){
        //TODO: Logger
        log(LOG_ERROR,"%s","ERROR. No more connections are allowed right now. Try again later\n");
        return -1;
    }

    clients[idx].socket = socket_fd;
    clients[idx].active = true;
    log(INFO,"Estoy por hacer store del socket_fd: %d que se va a guardar en el client[%d]", clients[idx].socket, idx);

    return idx;

}

int get_user_buffer_idx(connection * clients){
    for (int i = 0 ; i < MAX_CLIENTS ; i++){
        if (!clients[i].active)
            return i;
    }
    return -1;
}