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
#define MAX_PATH 4096
#define MAX_USERNAME 128

struct statistics {
    unsigned long historical_connections;
    unsigned long concurrent_connections;
    unsigned long transferred_bytes;
};


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
    char path[MAX_PATH]; //TODO: buscar tamano de los mails
    int size;
    bool to_delete;
}mail;

typedef struct {
    char mail_dir[MAX_PATH];
    size_t byte_size;
    size_t dim;
    mail * mails;
    int rtrv_fd; // fd of mail to retrieve
    size_t idx;
} inbox_state;

typedef struct {
    bool requested;
    bool auth;
    char username[MAX_USERNAME];
    int username_index; // if requested true then index holds the server idx for the username. O(1) pass check

    inbox_state inbox;
} user_state;



typedef struct command_buff{
    struct buffer retr_mail_buffer;
    char mail_buffer[MAX_BUFF_SIZE];
    char command[MAX_COMMAND_BUFF];
    int command_index;
    char args[MAX_COMMAND_BUFF];
    int args_index;
    bool has_error;
    bool has_finished;
    bool ok;
    char crlf;

    int connection_fd;
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

stm_states trans_capa(struct selector_key * key);
stm_states auth_capa(struct selector_key * key);
stm_states capa_write(struct selector_key * key, stm_states state);

stm_states noop(struct selector_key * key);
stm_states noop_write(struct selector_key * key);

stm_states auth_quit(struct selector_key * key);
stm_states trans_quit(struct selector_key * key);
stm_states quit_writ(struct selector_key * key, stm_states state);

stm_states pop_stat(struct selector_key * key);
stm_states stat_write(struct selector_key * key);

#endif
