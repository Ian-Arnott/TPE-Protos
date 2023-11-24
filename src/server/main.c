/**
 * main.c - servidor proxy socks concurrente
 *
 * Interpreta los argumentos de línea de comandos, y monta un socket
 * pasivo.
 *
 * Todas las conexiones entrantes se manejarán en éste hilo.
 *
 * Se descargará en otro hilos las operaciones bloqueantes (resolución de
 * DNS utilizando getaddrinfo), pero toda esa complejidad está oculta en
 * el selector.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>   // socket
#include <sys/socket.h>  // socket
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdbool.h>
// #include "../selector/selector.h"
#include "../args/args.h"
#include "../clients/client.h"
#include "pop3.h"

static bool done = false;

struct popargs args = {0};

static int setup_ipv4_tcp_socket(unsigned long port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(args.pop_port);

    const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server < 0) {
        return -1;        
    }
    // TODO: logger
    fprintf(stdout, "Listening IPv4 on TCP port %lu\n", port);


    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if(bind(server, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return -1;
    }

    if (listen(server, 20) < 0) {
        return -1;
    }
    return server;
}

static int setup_ipv4_udp_socket(unsigned long port) {
    struct sockaddr_in address;
    socklen_t address_length = sizeof(address);

    memset(&address, 0, address_length);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    int client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (client < 0) {
        return -1;
    }

    if (bind(client, (struct sockaddr *) &address, address_length) < 0) {
        return -1;
    }

    fprintf(stdout, "Listening IPv4 on UDP port %lu\n", port);
    return client;
}

static int setup_ipv6_udp_socket(unsigned long port) {
    struct sockaddr_in6 address;
    socklen_t address_length = sizeof(address);

    memset(&address, 0, address_length);
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(port);

    int client = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (client < 0) {
        return -1;
    }

    setsockopt(client, IPPROTO_IPV6, IPV6_V6ONLY, &(int){ 1 }, sizeof(int));

    if (bind(client, (struct sockaddr *) &address, address_length) < 0) {
        return -1;
    }

    fprintf(stdout, "Listening IPv6 on UDP port %lu\n", port);
    return client;
}

static int setup_ipv6_tcp_socket(unsigned long port) {
    // Storage for socket address
    struct sockaddr_in6 address;
    socklen_t address_length = sizeof(address);

    memset(&address, 0, address_length);
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(port);

    int server = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (server < 0) {
        return -1; 
    }

    // TODO: logger
    fprintf(stdout, "Listening IPv6 on TCP port %lu\n", port);

    setsockopt(server, IPPROTO_IPV6, IPV6_V6ONLY, &(int){ 1 }, sizeof(int));
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    if (bind(server, (struct sockaddr *) &address, address_length) < 0) {
        return -1;
    }

    if (listen(server, 20) < 0) {
        return -1;
    }

    return server;
}

static void
sigterm_handler(const int signal) {
    printf("signal %d, cleaning up and exiting\n",signal);
    done = true;
}

int
main(const int argc, const char **argv) {

    int ipv4_server_socket = -1;
    int ipv6_server_socket = -1;
    int ipv4_client_socket = -1;
    int ipv6_client_socket = -1;

    connection clients[MAX_CLIENTS];
    memset(&clients,0,sizeof(clients));

    parse_args(argc, argv, &args);

    printf("Estos son los argumentos.\n Puerto: %d. \n", args.pop_port);
    for (unsigned int i = 0; i < args.user_count; i++)
    {
        printf("User %d : %s : %s\n", i, args.users[i].name, args.users[i].pass);
    }

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    close(STDIN_FILENO);


    char        *err_msg = NULL;
    selector_status   ss      = SELECTOR_SUCCESS;
    fd_selector selector      = NULL;

    ipv4_server_socket = setup_ipv4_tcp_socket(args.pop_port);
    if (ipv4_server_socket < 0)
    {
        err_msg = "Error setting up IPv4 TCP socket";
        goto finally;
    }    
    ipv6_server_socket = setup_ipv6_tcp_socket(args.pop_port);
    if (ipv6_server_socket < 0)
    {
        err_msg = "Error setting up IPv6 TCP socket";
        goto finally;
    }
    ipv4_client_socket = setup_ipv4_udp_socket(args.mng_port);
    if (ipv4_client_socket < 0) {
        err_msg = "Error setting up IPv4 UDP socket";
        goto finally;
    }
    ipv6_client_socket = setup_ipv6_udp_socket(args.mng_port);
    if (ipv6_client_socket < 0) {
        err_msg = "Error setting up IPv6 UDP socket";
        goto finally;
    }
    
    
    // registrar sigterm es útil para terminar el programa normalmente.
    // esto ayuda mucho en herramientas como valgrind.
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT,  sigterm_handler);

    if(selector_fd_set_nio(ipv4_server_socket) == -1) {
        err_msg = "getting server socket flags";
        goto finally;
    }    
    if(selector_fd_set_nio(ipv6_server_socket) == -1) {
        err_msg = "getting server socket flags";
        goto finally;
    }
    if(selector_fd_set_nio(ipv4_client_socket) == -1) {
        err_msg = "getting server socket flags";
        goto finally;
    }
    if(selector_fd_set_nio(ipv6_client_socket) == -1) {
        err_msg = "getting server socket flags";
        goto finally;
    }

    // SELECTOR
    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec  = 10,
            .tv_nsec = 0,
        },
    };
    if(0 != selector_init(&conf)) {
        err_msg = "initializing selector";
        goto finally;
    }
    selector = selector_new(1024);
    if(selector == NULL) {
        err_msg = "unable to create selector";
        goto finally;
    }

    const struct fd_handler pop3_handler = {
        .handle_read       = accept_connection_handler, // antes habia otra cosa
        .handle_write      = NULL,
        .handle_close      = NULL, // nada que liberar
    };

    const struct fd_handler client_handler = {
        .handle_read       = accept_client_handler, // antes habia otra cosa
        .handle_write      = NULL,
        .handle_close      = NULL, // nada que liberar
    };

    

    ss = selector_register(selector, ipv4_server_socket, &pop3_handler,OP_READ, (void *) &args);
    if(ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd for IPv4";
        goto finally;
    }
    ss = selector_register(selector, ipv6_server_socket, &pop3_handler,OP_READ, (void *) &args);
    if(ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd for IPv6";
        goto finally;
    }
    ss = selector_register(selector, ipv4_client_socket, &client_handler,OP_READ, (void *) &args);
    if(ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd for IPv4";
        goto finally;
    }
    ss = selector_register(selector, ipv6_client_socket, &client_handler,OP_READ, (void *) &args);
    if(ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd for IPv6";
        goto finally;
    }

    // if not done
    for(;!done;) {
        err_msg = NULL;
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            err_msg = "serving";
            goto finally;
        }
    }
    if(err_msg == NULL) {
        err_msg = "closing";
    }

    int ret = 0;
    
    finally:
    if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (err_msg == NULL) ? "": err_msg,
                                  ss == SELECTOR_IO
                                      ? strerror(errno)
                                      : selector_error(ss));
        ret = 2;
    } else if(err_msg) {
        perror(err_msg);
        ret = 1;
    }
    if(selector != NULL) {
        selector_destroy(selector);
    }
    selector_close();

    if(ipv4_server_socket >= 0) {
        close(ipv4_server_socket);
    }
    if(ipv6_server_socket >= 0) {
        close(ipv6_server_socket);
    }
    return ret;
}