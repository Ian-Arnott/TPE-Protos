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