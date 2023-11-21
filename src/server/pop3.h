#ifndef POP3_H_
#define POP3_H_

#include <stdbool.h>
#include "../selector/selector.h"
#include "../selector/stm.h"

#define MAX_CLIENTS 512


typedef enum {
    AUTHORIZATION = 0,
    TRANSACTION,
    ERROR,
    QUIT,
    STM_STATES_COUNT
} stm_states;

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
    struct state_machine stm;

    bool active;
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
