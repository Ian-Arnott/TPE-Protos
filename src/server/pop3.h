#ifndef POP3_H_
#define POP3_H_

#include <stdbool.h>
#include "../selector/selector.h"
#include "../selector/stm.h"
#include "../logger/logger.h"
#include "../buffer/buffer.h"
#include "../parser/parser.h"

#define MAX_CLIENTS 512
#define MAX_BUFF_SIZE 1024


typedef enum {
    ANY_CHARACTER,
    CR,
    LF,
    DOT
} crlf_flag;

typedef enum {
    AUTHORIZATION = 0,
    TRANSACTION,
    ERROR,
    QUIT,
    STM_STATES_COUNT
} stm_states;

typedef enum {
    COMMAND = 0,
    ARGUMENT,
    END,
    PARSER_STATES_COUNT
} parser_states;

// USER
typedef struct {
    char content[2048]; //TODO: buscar tamano de los mails
    int size;
    bool to_delete;
}mail;

typedef struct {
    int idx;
    int size;
    mail * ptr;
} inbox_state;

typedef struct {
    bool active;

    bool auth;
    char username[128];
    inbox_state * inbox;
    int inbox_size;
    int socket_fd;
} user_state;

typedef struct connection{
    int socket;
    bool active;

    struct state_machine stm;

    struct buffer command_buffer;
    char command_buff[MAX_BUFF_SIZE];
    struct buffer server_buffer;
    char server_buff[MAX_BUFF_SIZE];

    struct parser * parser;
    stm_states last_states;

    // comando current_comando;

}connection;

typedef struct comando
{
    // char comando[MAX_COMMAND_LEN + 1];
    // char argumento[MAX_ARG_LEN + 1];
    size_t comando_len;
    size_t arg_len;

    bool ended;
    bool error;
    size_t res_index;
    int mail_fd;
    int connec_fd;
    crlf_flag crlf_flag;
    
};



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
// struct connection_data {
//     struct buffer in_buffer_object;
//     char in_buffer[BUFFER_SIZE];
//     struct buffer out_buffer_object;
//     char out_buffer[BUFFER_SIZE];
//     struct parser * parser;
//     struct state_machine stm;

//     struct session current_session;
//     struct command current_command;
//     stm_states last_state;
// };

// HANDLERS
void accept_connection_handler(struct selector_key * key);
void client_close(struct selector_key *key);
void client_write(struct selector_key *key);
void client_read(struct selector_key *key);
void user_write_handler(struct selector_key * key);

// UTILS
int store_connection(int socket_fd, connection * clients);
int get_user_buffer_idx(connection * clients);

/**
 * USER command
*/
int user(user_state * user, char * args);

/**
 * PASS command
*/
int pass(user_state * user, char * args);

/**
 * LIST command
*/
void list(user_state * user);

/**
 * RETR command
*/
int retr(user_state * user, int mail_id);

/**
 * DELE command
*/
int dele(user_state * user, int mail_id);

/*
 * REST command
*/
int rset(user_state * user);

/*
 * NOOP command
*/
int noop();

/*
 * QUIT command
*/
int quit(user_state * user);

/*
 *  STAT command
*/
int stat(); // basically the help command
#endif
