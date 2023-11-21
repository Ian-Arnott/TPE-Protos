/**
 * pop3.c  - 
 */
#include<stdio.h>
#include <stdlib.h>  // malloc
#include <string.h>  // memset
#include <assert.h>  // assert
#include <errno.h>
#include <time.h>
#include <unistd.h>  // close
#include <pthread.h>

#include <arpa/inet.h>

// #include "hello.h"
// #include "request.h"
#include "../buffer/buffer.h"
#include "pop3.h"
#include "pop_utils.h"
#include "../net/netutils.h"

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
    const int client = accept(key->fd, (struct sockaddr *)&client_addr, &client_addr_len);
    
    connection * clients = key->data;

    if (client == -1) {
        goto fail;
    }

    // Convierte en non blocking el socket nuevo.
    if (selector_fd_set_nio(client) == -1) {
        goto fail;
    }

    // Almacenar la conexión (suponiendo que esta función existe y funciona correctamente).
    int new_idx = store_connection(client, (connection *)key->data);
    if ( new_idx < 0) {
        goto fail;
    }
    
    clients[new_idx].stm.states = stm_states_table;
    clients[new_idx].stm.initial = AUTHORIZATION;
    clients[new_idx].stm.max_state = STM_STATES_COUNT;
    stm_init(&clients[new_idx].stm);

    // buffer_init(&connection->in_buffer_object, BUFFER_SIZE, (uint8_t *) connection->in_buffer);
    // buffer_init(&connection->out_buffer_object, BUFFER_SIZE, (uint8_t *) connection->out_buffer);
    // connection->parser = parser_init(parser_no_classes(), &parser_definition);
    // connection->last_state = -1;
    // connection->current_command.mail_fd = -1;
    // buffer_init(&connection->current_command.mail_buffer_object, BUFFER_SIZE, (uint8_t *) connection->current_command.mail_buffer);
    // connection->current_session.mails = calloc(args.max_mails, sizeof(struct mail));
    // connection->current_session.requested_quit = false;

    // Registrar el nuevo socket cliente con el selector.
    if (selector_register(key->s, client, &client_handler, OP_READ, &clients[new_idx]) != SELECTOR_SUCCESS) {
        goto fail;
    }

    write(STDOUT_FILENO, "Stored User\n", 13);
    return;

fail:
    if (client != -1) {
        close(client);
    }
}

// void accept_connection_handler(struct selector_key *key){
//     struct sockaddr_storage client_addr;
//     socklen_t client_addr_len = sizeof(client_addr);

//     const int client = accept(key->fd, (struct sockaddr *) &client_addr, &client_addr_len);

//     if (client == -1) {
//         goto fail;
//     }
//     // convierte en  non blocking el socket nuevo.
//     int client_fd  = selector_fd_set_nio(client);
//     if (client_fd == -1) {
//         goto fail;
//     }
    

//     if (store_connection(client, (connection *) key->data) == 1){
//         goto fail;
//     }
    
//     write(STDOUT_FILENO, "Stored User\n", 13);
    
//     return;
    

//     fail:
//     if (client != -1) {
//         close(client);
//     }

// }

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
        printf("ERROR. No more connections are allowed right now. Try again later\n");
        return -1;
    }

    clients[idx].socket = socket_fd;
    clients[idx].active = true;
    printf("Estoy por hacer store del socket_fd: %d que se va a guardar en el client[%d]", clients[idx].socket, idx);

    return idx;

}

int get_user_buffer_idx(connection * clients){
    for (int i = 0 ; i < MAX_CLIENTS ; i++){
        if (!clients[i].active)
            return i;
    }
    return -1;
}