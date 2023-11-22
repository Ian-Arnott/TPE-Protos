#ifndef POP3_H_
#define POP3_H_

#include <stdbool.h>
#include "../selector/selector.h"
#include "../selector/stm.h"
#include "../logger/logger.h"
#include "../buffer/buffer.h"
#include "../parser/parser.h"
#include "../args/args.h"

#define MAX_CLIENTS 512
#define MAX_BUFF_SIZE 1024
#define MAX_COMMAND_BUFF 64


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
    int fd;
    int size;
} inbox_state;

typedef struct {
    bool requested;
    bool auth;
    char username[128];
    int username_index; // if requested true then index holds the server idx for the username. O(1) pass check

    // char maildir[128];
    int inbox_fd;
    int inbox_size;
    mail mails[];
    // int socket_fd;
} user_state;



typedef struct command_buff{
    char command[MAX_COMMAND_BUFF];
    char args[MAX_COMMAND_BUFF];
    bool has_error;
    bool has_finished;
}command_buff;

typedef struct connection{
    int socket;
    bool active;

    user_state user_data;

    struct state_machine stm;

    struct buffer command_buffer;
    char command_buff[MAX_BUFF_SIZE];
    struct buffer server_buffer;
    char server_buff[MAX_BUFF_SIZE];

    struct parser * parser;
    stm_states last_states;
    command_buff command;

}connection;



// HANDLERS
void accept_connection_handler(struct selector_key * key);
void client_close(struct selector_key *key);
void client_write(struct selector_key *key);
void client_read(struct selector_key *key);
void user_write_handler(struct selector_key * key);

// UTILS
int store_connection(int socket_fd, connection * clients);
int get_user_buffer_idx(connection * clients);

// COMMANDS
stm_states user(struct selector_key * key);
stm_states user_write(struct selector_key * key);

stm_states pass(struct selector_key * key);
stm_states pass_write(struct selector_key * key);

stm_states list(struct selector_key * key);
stm_states list_write(struct selector_key * key);

stm_states retr(struct selector_key * key);
stm_states retr_write(struct selector_key * key);

stm_states dele(struct selector_key * key);
stm_states dele_write(struct selector_key * key);

stm_states rset(struct selector_key * key);
stm_states rset_write(struct selector_key * key);

stm_states capa(struct selector_key * key);
stm_states capa_write(struct selector_key * key, stm_states state);

stm_states noop();
stm_states noop_write();

stm_states quit(struct selector_key * key);
stm_states quit_writ(struct selector_key * key, stm_states state);

stm_states stat();
stm_states stat_write();

#endif
