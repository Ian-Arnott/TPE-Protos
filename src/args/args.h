#ifndef ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8
#define ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8

#include <stdbool.h>

#include "../clients/auth.h"

#define MAX_USERS 1024
#define MAX_USER_LENGTH 64

// // USER
// typedef struct {
//     char content[2048]; //TODO: buscar tamano de los mails
//     int size;
//     bool to_delete;
// }mail;

// typedef struct {
//     int idx;
//     int size;
//     mail * ptr;
// } inbox_state;

// typedef struct {
//     // char read[1024]; // identificador de la instruccion
//     // char write[1024]; // identificadores de parametros
//     // int read_idx;
//     // int write_idx;
//     bool auth; 
//     char username[128];
//     inbox_state * inbox;
//     int inbox_size;
//     int socket_fd;
// } user_state;


struct users {
    char name[MAX_USER_LENGTH];
    char pass[MAX_USER_LENGTH];
    

    // struct para el estado del inbox al hacer login
};

struct doh {
    char           *host;
    char           *ip;
    unsigned short  port;
    char           *path;
    char           *query;
};

struct socks5args {
    char           *socks_addr; // localhost
    unsigned short  socks_port; // 110

    char *          mng_addr; // localhost
    unsigned short  mng_port; // 9090

    bool            disectors_enabled; // ???

    struct doh      doh;
    struct users    users[MAX_USERS];
};

struct popargs {
    char           *pop_addr; // localhost
    unsigned short  pop_port; // 110
    char *          mng_addr; // localhost
    unsigned short  mng_port; // 9090
    
    struct users    users[MAX_USERS];
    unsigned int    user_count;
};

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecución.
 */
void 
parse_args(const int argc, const char **argv, struct popargs *args);

#endif
