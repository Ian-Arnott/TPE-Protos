#ifndef POP3_H_
#define POP3_H_

#include <stdbool.h>

typedef struct {
    char read[1024]; // identificador de la instruccion
    char write[1024]; // identificadores de parametros
    int read_idx;
    int write_idx;
    bool auth; 
    char username[128];
    inbox_state * inbox;
    int socket_fd;
} user_state;

typedef struct {
    int idx;
    int size;
    mail * ptr;
} inbox_state;

typedef struct {
    char content[2048]; //TODO: buscar tamano de los mails
    int size;
    bool to_delete;
}mail;


// back
// ESTADO (casilla de mail)
    // MAILS
    // USERS
    // PASS
// DELE

// DELE // RSET (marca para no borrar todos los mensajes)
// back
//adol USER -> PASS -> LIST -> RETRV
//ian USER -> PASS ->PASS -> LIST

// USER -> USER -> PASS -> LIST -> PASS -> PASS -> RETRV -> LIST

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
int list(user_state * user);

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
