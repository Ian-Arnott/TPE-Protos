#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>

#include "args.h"

#define MAX_PORT 65535

// popargs args;

static unsigned short
port(const char *s) {
     char *end     = 0;
     const long sl = strtol(s, &end, 10);

     if (end == s|| '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
         fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
         exit(1);
         return 1;
     }
     return (unsigned short)sl;
}

static void
new_user(const char *s, struct users *user) {
    char *p = strchr(s, ':');
    if(p == NULL) {
        fprintf(stderr, "password not found\n");
        exit(1);
    } else {
        *p = 0;
        p++;
        strcpy(user->name, s);
        strcpy(user->pass, p);

    }

}

static void
version(void) {
    fprintf(stderr, "POP3 version 1.0\n"
                    "ITBA Protocolos de Comunicación 2023/2 -- Grupo 6\n"
                    "AQUI VA LA LICENCIA\n");
}

static void
usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s [OPTION]...\n"
        "\n"
        "   -h               Imprime la ayuda y termina.\n"
        "   -l <SOCKS addr>  Dirección donde servirá el proxy SOCKS.\n"
        "   -L <conf  addr>  Dirección donde servirá el servicio de management.\n"
        "   -p <SOCKS port>  Puerto entrante conexiones SOCKS.\n"
        "   -P <conf port>   Puerto entrante conexiones configuracion\n"
        "   -u <name>:<pass> Usuario y contraseña de usuario que puede usar el proxy. Hasta 10.\n"
        "   -v               Imprime información sobre la versión versión y termina.\n"
        "\n"
        "   --doh-ip    <ip>    \n"
        "   --doh-port  <port>  XXX\n"
        "   --doh-host  <host>  XXX\n"
        "   --doh-path  <host>  XXX\n"
        "   --doh-query <host>  XXX\n"

        "\n",
        progname);
    exit(1);
}


void 
parse_args(const int argc, const char **argv, struct popargs *args) {
    memset(args, 0, sizeof(*args)); // sobre todo para setear en null los punteros de users

    args->pop_addr = "127.0.0.1";
    args->pop_port = 1110;
// ACA VAMOS A CAMBIAR EL PUERTO 110
    args->mng_addr   = "127.0.0.1";
    args->mng_port   = 8080;

    if ( argc <= 2){
        log(LOG_ERROR,"%s", "ERROR Invalid arguments, usage <PORT> -u user1:password1 -u user2:password2 ...");
		exit(1);
	}

    // int c;
    // int nusers = 0;
    
// TODO: parse users and create their data

    bool error = false;
	for ( int i = 1; i < argc && !error; i++){
        
		if(strcmp(argv[i],"-u") == 0){
			if ( i + 1 < argc){
				args->user_count++;
				if ( args->users == NULL){
                    // TODO: Logger
					// log(FATAL,"%s","NO MEMORY");
				}
                if ( !parse_user_and_password(argv[i + 1])){
                    error = true;
                }
				new_user(argv[i + 1],&args->users[args->user_count - 1]);
				i++;
			} else {
                // TODO: Logger
				log(LOG_ERROR,"%s","Invalid Usage: format -u must be followed by user:pass\n");
				error = true;
			}
		}else if (i == 1){
            args->pop_port = port(argv[i]);
        } // PORT
	}
    if (error){
        // TODO: Logger
        log(LOG_ERROR,"%s", ". Invalid arguments\n");
        exit(1);
    }

    
    // while (true) {
    //     int option_index = 0;
    //     static struct option long_options[] = {
    //         { "doh-ip",    required_argument, 0, 0xD001 },
    //         { "doh-port",  required_argument, 0, 0xD002 },
    //         { "doh-host",  required_argument, 0, 0xD003 },
    //         { "doh-path",  required_argument, 0, 0xD004 },
    //         { "doh-query", required_argument, 0, 0xD005 },
    //         { 0,           0,                 0, 0 }
    //     };

    //     c = getopt_long(argc, argv, "hl:L:Np:P:u:v", long_options, &option_index);
    //     if (c == -1)
    //         break;

    //     switch (c) {
    //         case 'h':
    //             usage(argv[0]);
    //             break;
    //         case 'l':
    //             args->pop_addr = optarg;
    //             break;
    //         case 'L':
    //             args->mng_addr = optarg;
    //             break;
    //         case 'P':
    //             args->mng_port   = port(optarg);
    //             break;
    //         case 'u':
    //             if(nusers >= MAX_USERS) {
    //                 fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
    //                 exit(1);
    //             } else {
    //                 user(optarg, args->users + nusers);
    //                 nusers++;
    //             }
    //             break;
    //         case 'v':
    //             version();
    //             exit(0);
    //             break;
    //         default:
    //             fprintf(stderr, "unknown argument %d.\n", c);
    //             exit(1);
    //     }

    // }
    // if (optind < argc) {
    //     fprintf(stderr, "argument not accepted: ");
    //     while (optind < argc) {
    //         fprintf(stderr, "%s ", argv[optind++]);
    //     }
    //     fprintf(stderr, "\n");
    //     exit(1);
    // }

    
}

bool parse_user_and_password(const char * string){
    int idx; // current char
    int length = 0;
    bool flag = false; // found :
    for (idx = 0; string[idx] != 0 ; idx++){
        if (!flag && string[idx] == ':'){
            flag = true;
            length = 0; //reset the length counter
        }else{
            if (length >= MAX_USER_LENGTH || !isalnum(string[idx])){
                return false;
            }
        }
        length++;
    }
    if (idx < 1 || flag == false){ // idx < 2 for password length // flag to check : input
        return false;
    }
    return true;
}