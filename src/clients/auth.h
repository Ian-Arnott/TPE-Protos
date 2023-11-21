#ifndef AUTH_H_
#define AUTH_H_

#include <stdbool.h>
#include "../args/args.h"
#include "../selector/stm.h"

#define MAX_CLIENTS 512
typedef struct connection{
    int socket;
    struct state_machine stm;

    bool active;
}connection;



// me conecto
// 

// typedef struct user_data{
//     struct command_list * command_list;
//     buffer output_buff;
//     pop_state session_state;
//     client_state client_state;
//     int socket;
//     login_info login_info;
//     void *currentCommand; //command currently executing
//     command_execute_state commandState; //command execution status (tells if you can execute a new command)
//     mailCache * mailCache;
// } user_data;

bool parse_user_and_password( const char * string);

int store_connection(int socket_fd, connection * clients);
int get_user_buffer_idx(connection * clients);

#endif