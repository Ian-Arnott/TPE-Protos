#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>

#define BUFLEN 512  // Tamaño máximo del buffer

void die(char *s) {
    perror(s);
    exit(1);
}

int main(int argc, char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    if (argc < 3) {
        fprintf(stderr,"Usage: %s <SERVER-PORT> <ADMIN-PASS> -l -s -d <newmaildir> -u <newuser:pass> ... -r <user> ... \n", argv[0]);
        exit(1);
    } else if (isalpha(argv[1][0]))
    {
        fprintf(stderr,"<SERVER-PORT> Must be a number\n");
        fprintf(stderr,"Usage: %s <SERVER-PORT> <ADMIN-PASS> -l -s -d <newmaildir> -u <newuser:pass> ... -r <user> ... \n", argv[0]);
        exit(1);
    }
    

    unsigned long port = strtoul(argv[1], NULL, 10);
    
    char args[BUFLEN] = {0};
    char response[BUFLEN] = {0};
    for (int i = 2; i < argc; i++)
    {

        strcat(args, argv[i]);
        strcat(args, " ");
    }
    strcat(args, "\0");
    

    int client_socket;
    client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(client_socket < 0){
        fprintf(stderr,"Error while connecting to server\n");
        exit(1);
    }

    
    struct sockaddr_in address;
    unsigned int address_length = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    address.sin_addr.s_addr = INADDR_ANY;

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        printf("Unable to setup server socket\n");
        close(client_socket);
        exit(1);        
    }

    if (sendto(client_socket, args, strlen(args), 0, (struct sockaddr *) &address, address_length) < 0) {
        fprintf(stderr,"Error while sending request\n");
        exit(1);
    }

    if (recvfrom(client_socket, response, sizeof(response), 0, (struct sockaddr *) &address, &address_length) < 0) {
        fprintf(stderr,"Error while receiving request\n");
        exit(1);
    }

    fprintf(stdout,"%s",response);


    close(client_socket);

    return 0;
}
