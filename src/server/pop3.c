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

// #include "stm.h"
#include "pop3.h"
#include "../net/netutils.h"

void accept_connection_handler(struct selector_key *key){
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    const int client = accept(key->fd, (struct sockaddr *) &client_addr, &client_addr_len);

    if (client == -1) {
        goto fail;
    }
    // convierte en  non blocking el socket nuevo.
    if (selector_fd_set_nio(client) == -1) {
        goto fail;
    }


    fail:
    if (client != -1) {
        close(client);
    }

}

void user_write_handler(struct selector_key * key){
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char * to_print = "Hello from the other side";
    int bytes_to_send = strlen(to_print);

    // write(key->fd, (char *)key->data, strlen((char *) key->data ));
    sendto(key->fd, to_print, bytes_to_send, 0, (struct sockaddr *) &client_addr, client_addr_len);
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