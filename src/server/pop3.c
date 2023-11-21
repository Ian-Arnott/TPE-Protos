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
#include "../clients/auth.h"
#include "pop3.h"
#include "../net/netutils.h"

// Ejemplo de fd_handler para el cliente
static const struct fd_handler client_handler = {
    .handle_read = client_read,   // Función que maneja la lectura.
    .handle_write = client_write,         // Si no necesitas manejar escritura inmediatamente.
    .handle_close = client_close, // Función que maneja el cierre.
    // .handle_block = NULL,       // Si tienes un handler para operaciones bloqueantes.
};

void client_close(struct selector_key *key) {
}

void client_read(struct selector_key *key) {
    stm_handler_read(&((connection *)key->data)->stm, key);
}

void client_write(struct selector_key *key) {
    stm_handler_read(&((connection *)key->data)->stm, key);
}

void accept_connection_handler(struct selector_key *key) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    const int client = accept(key->fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client == -1) {
        goto fail;
    }

    // Convierte en non blocking el socket nuevo.
    if (selector_fd_set_nio(client) == -1) {
        goto fail;
    }

    // Almacenar la conexión (suponiendo que esta función existe y funciona correctamente).
    if (store_connection(client, (connection *)key->data) == 1) {
        goto fail;
    }

    // Registrar el nuevo socket cliente con el selector.
    if (selector_register(key->s, client, &client_handler, OP_READ, key->data) != SELECTOR_SUCCESS) {
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