#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <strings.h>
#include <dirent.h>
#include "args.h"

#define MAX_PORT 65535

// popargs args;

static bool is_valid_directory(const char *path) {
    // Try to open the directory
    DIR *dir = opendir(path);

    // Check if the directory can be opened
    if (dir != NULL) {
        // Directory opened successfully, so close it and return 1 (true)
        closedir(dir);
        return true;
    } else {
        // Directory couldn't be opened, so return 0 (false)
        return false;
    }
}

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
usage(const char * progname) {
    fprintf(stderr,
        "Usage: %s [OPTIONS]...\n"
        "\n"
        "   -h                               This help message.\n\n"
        "   -d <maildir>                     Path to directory where it'll find all users with their mails.\n\n"
        "   -p <pop3 server port>            Port for POP3 server connections.\n\n"
        "   -P <configuration server port>   Port for configuration client connections\n\n"
        "   -u <user>:<password>             User and password for a user which can use the POP3 server. Up to 10.\n\n"
        "   -t <token>                       Authentication token for the client.\n\n"
        "   -v                               Prints version information.\n"
        "\n",
        progname);
    exit(1);
}

// 
void 
parse_args(const int argc, const char **argv, struct popargs *args) {
    memset(args, 0, sizeof(*args)); // sobre todo para setear en null los punteros de users

    args->pop_addr = "127.0.0.1";
    args->pop_port = 1110;

    args->mng_addr   = "127.0.0.1";
    args->mng_port   = 9090;

    strcpy(args->mail_directory, "./mails/");
    // args->mail_directory = "mails";

    // MAX MAILS per client

    args->max_mails = DEFAULT_MAX_MAILS;

    if ( argc <= 5){
        log(LOG_ERROR,"%s", "ERROR Invalid arguments, usage: <POP3-PORT> <SERVER-PORT> <ADMIN-PASS> -u user1:password1 -u user2:password2 ...");
		exit(1);
	}

    // int c;
    // int nusers = 0;
    
// TODO: parse users and create their data

//
    bool error = false;
	for ( int i = 1; i < argc && !error; i++){

        if(strcmp(argv[1],"-h") == 0)
        {
            const char * program = "pop3d";
            usage(program);
        }
        
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
		} else if(strcmp(argv[i],"-d") == 0)
        {
            if ( i + 1 < argc && is_valid_directory(argv[i+1]) == true){
				strcpy(args->mail_directory, argv[i+1]);
				i++;
			} else {
                // TODO: Logger
				log(LOG_ERROR,"%s","Invalid Usage: format -d must be followed by maildir -- maildir might me invalid\n");
				error = true;
			}   
        } else if(strcmp(argv[i],"-p") == 0)
        {
            if ( i + 1 < argc){
                args->pop_port = port(argv[i+1]);
				i++;
			} else {
                // TODO: Logger
				log(LOG_ERROR,"%s","Invalid Usage: format -p must be followed by a port\n");
				error = true;
			}   
        } else if(strcmp(argv[i],"-P") == 0)
        {
            if ( i + 1 < argc){
                args->mng_port = port(argv[i+1]);
				i++;
			} else {
                // TODO: Logger
				log(LOG_ERROR,"%s","Invalid Usage: format -P must be followed by a port\n");
				error = true;
			}   
        } else if(strcmp(argv[i],"-t") == 0)
        {
            if ( i + 1 < argc){
                strcpy(args->admin, argv[i+1]);
				i++;
			} else {
                // ./pop3d -d dir -u user:pass -P 9090 -p 1110 -t pass 
                // TODO: Logger
				log(LOG_ERROR,"%s","Invalid Usage: format -t must be followed by a token\n");
				error = true;
			}   
        }
        else if(strcmp(argv[i],"-v") == 0)
        {
            version();
        }
        
	}
    if (error){
        // TODO: Logger
        log(LOG_ERROR,"%s", ". Invalid arguments\n");
        exit(1);
    }
    
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