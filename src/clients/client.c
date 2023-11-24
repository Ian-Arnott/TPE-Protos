#include <stdio.h>
#include <stdlib.h>  // malloc
#include <string.h>  // memset
#include <assert.h>  // assert
#include <errno.h>
#include <time.h>
#include <unistd.h>  // close
#include <pthread.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "client.h"
#include "client_utils.h"
#include "../parser/parser.h"
#include "../logger/logger.h"
#include "../args/args.h"
#include "../server/pop3.h"
extern struct popargs args;
extern struct statistics stats;

static int remove_user(const char *username) {
    int index = -1;
    for (size_t i = 0; i < args.user_count; i++) {
        if (strcmp(args.users[i].name, username) == 0) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return -1;
    }

    for (size_t i = index; i < args.user_count - 1; i++) {
        strcpy(args.users[i].name, args.users[i + 1].name);
        strcpy(args.users[i].pass, args.users[i + 1].pass);
    }

    memset(args.users[args.user_count - 1].name, 0, sizeof(args.users[args.user_count - 1].name));
    memset(args.users[args.user_count - 1].pass, 0, sizeof(args.users[args.user_count - 1].pass));

    args.user_count--;

    return 0; // Success
}

static void parse_client(char *buffer, char *write_buffer, size_t n) {
    buffer[n] = '\0';
    bool logged_in = false;
    char *token = strtok(buffer, " ");
    while (token != NULL) {
        if (strcmp(token, args.admin) == 0 && !logged_in) {
            strcat(write_buffer, "Admin Verified.\n");
            logged_in = true;
        } else if (!logged_in)
        {
            write_buffer[0] = 0;
            strcat(write_buffer, "Error: Incorrect admin pass: ");
            strcat(write_buffer, token);
            strcat(write_buffer, "\n");
            return;
        }
        
        
        if (strcmp(token, "-s") == 0) {
            strcat(write_buffer, "POP3 Server Stats:\n");
            char buffer[50];

            sprintf(buffer, "Historical: %lu\n", stats.historical);
            strcat(write_buffer, buffer);

            sprintf(buffer, "Concurrent: %lu\n", stats.concurrent);
            strcat(write_buffer, buffer);

            sprintf(buffer, "Transfered Bytes: %lu\n\n", stats.transferred_bytes);
            strcat(write_buffer, buffer);
        } else if (strcmp(token, "-l") == 0) {
            strcat(write_buffer, "User List:\n");
            for (size_t i = 0; i < args.user_count; i++)
            {
                strcat(write_buffer, args.users[i].name);
                strcat(write_buffer,"\n");
            }
            strcat(write_buffer,"\n");
        } else if (strcmp(token, "-u") == 0) {

            token = strtok(NULL, " ");
            if (token != NULL) {
                char *username = strtok(token, ":");
                char *password = strtok(NULL, ":");
                if (username != NULL && password != NULL ) {
                    strcpy(args.users[args.user_count].name,username);
                    strcpy(args.users[args.user_count++].pass,password);
                    strcat(write_buffer, "Added User: ");
                    strcat(write_buffer, username);
                    strcat(write_buffer, "\n");
                } else{
                    write_buffer[0] = 0;
                    strcat(write_buffer, "Error: add user usage: -u username:pass\n");
                    return;
                }
            } else
            {
                    write_buffer[0] = 0;
                    strcat(write_buffer, "Error: add user usage: -u username:pass\n");
                    return;
            }
        } else if (strcmp(token, "-r") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL)
            {
                char buffer[50];
                char *username = strtok(token, ":");
                if (username != NULL)
                {
                    if (remove_user(username) < 0)
                    {
                        write_buffer[0] = 0;
                        sprintf(buffer, "Error: Failed To Removed User: %s\n\n", token);
                        strcat(write_buffer, buffer);
                        return;
                    }
                    sprintf(buffer, "Removed User: %s\n\n", token);
                    strcat(write_buffer, buffer);
                } else {
                    write_buffer[0] = 0;
                    strcat(write_buffer, "Error: remove user usage: -r username:\n");
                    return;
                }
            } else {
                    write_buffer[0] = 0;
                    strcat(write_buffer, "Error: remove user usage: -r username:\n");
                    return;
            }
            
        } else if (strcmp(token, "-c") == 0) {
            // Handle -c option, change user's password
            // Example: change_password();
            token = strtok(NULL, " ");
            if (token != NULL) {
                // Assuming the format is username:newpassword
                char *username = strtok(token, ":");
                char *newpassword = strtok(NULL, ":");
                if (username != NULL && newpassword != NULL) {
                    // Example: change_password(username, newpassword);
                }
            }
        } else if (strcmp(token, "-d") == 0) {
            // Handle -d option, set a new maildir in args
            // Example: set_maildir();
            token = strtok(NULL, " ");
            if (token != NULL) {
                // Example: set_maildir(token);
            }
        } else if (strcmp(token, "-p") == 0) {
            // Handle -p option, change the port value in args
            // Example: change_port();
            token = strtok(NULL, " ");
            if (token != NULL) {
                // Example: change_port(atoi(token));
            }
        } else if (strcmp(token, "-P") == 0) {
            // Handle -P option, change the management port value in args
            // Example: change_management_port();
            token = strtok(NULL, " ");
            if (token != NULL) {
                // Example: change_management_port(atoi(token));
            }
        }

        token = strtok(NULL, " ");
    }
}

void accept_client_handler(struct selector_key * key) {
    struct sockaddr_in client;
    unsigned int client_size = sizeof(client);

    char read_buffer[MAX_CLIENT_BUFFER] = {0};
    char write_buffer[MAX_CLIENT_BUFFER] = {0};

    ssize_t n = recvfrom(key->fd, read_buffer, MAX_CLIENT_BUFFER, 0, (struct sockaddr *) &client, &client_size);
    if (n <= 0) {
        return;
    }

    log(INFO, "UDP Client - Recived Message: %s", read_buffer);

    parse_client(read_buffer, write_buffer, n);
    
    sendto(key->fd, write_buffer, strlen(write_buffer), 0, (struct sockaddr *) &client, client_size);

    log(INFO, "UDP Client - Sent Message: %s", write_buffer);
}