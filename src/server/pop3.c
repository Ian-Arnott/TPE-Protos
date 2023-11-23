/**
 * pop3.c  - 
 */
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


// #include "hello.h"
// #include "request.h"
#include "../buffer/buffer.h"
#include "pop3.h"
#include "../net/netutils.h"
#include "pop3_stm_handlers.h"
#include "pop_utils.h"

// ---- MAIN ARGS -----
extern struct popargs args;

struct state_definition stm_states_table[] = {
        {
                .state = AUTHORIZATION,
                .on_arrival = authorization_arrival,
                .on_departure = authorization_departure,
                .on_read_ready = authorization_read,
                .on_write_ready = authorization_write
        },
        {
                .state = TRANSACTION,
                .on_arrival = transaction_arrival,
                .on_departure = transaction_departure,
                .on_read_ready = transaction_read,
                .on_write_ready = transaction_write
        },
        {
                .state = ERROR,
                .on_arrival = error_arrival,
                .on_departure = error_departure,
                .on_read_ready = error_read,
                .on_write_ready = error_write
        },
        {
                .state = QUIT,
                .on_arrival = quit_arrival,
                .on_departure = quit_departure,
                .on_read_ready = quit_read,
                .on_write_ready = quit_write
        }
};

static const struct parser_state_transition parser_command[] = {
        {.when = ' ', .dest = ARGUMENT, .act1 = parser_command_space},
        {.when = '\r', .dest = END, .act1 = parser_command_return},
        {.when = ANY, .dest = COMMAND, .act1 = parser_command_any}
};

static const struct parser_state_transition parser_argument[] = {
        {.when = '\r', .dest = END, .act1 = parser_argument_return},
        {.when = ANY, .dest = ARGUMENT, .act1 = parser_argument_any}
};

static const struct parser_state_transition parser_end[] = {
        {.when = '\n', .dest = COMMAND, .act1 = parser_end_enter},
        {.when = ANY, .dest = COMMAND, .act1 = parser_end_any}
};

static const struct parser_state_transition * parser_table[] = {
        parser_command,
        parser_argument,
        parser_end
};

static const size_t states_dim[] = {
        sizeof(parser_command) / sizeof(parser_command[0]),
        sizeof(parser_argument) / sizeof(parser_argument[0]),
        sizeof(parser_end) / sizeof(parser_end[0]),
};

struct parser_definition parser_deff = {
    .states = parser_table,
    .states_count = PARSER_STATES_COUNT,
    .start_state = COMMAND,
    .states_n = states_dim
};



static const struct fd_handler client_handler = {
    .handle_read = client_read,
    .handle_write = client_write,
    .handle_close = client_close,
};

static int get_idx_of_connection(int socket, connection * clients){
    for (int i = 0 ; i < MAX_CLIENTS ; i++){
        if (clients[i].socket == socket && clients[i].active){
            return i;
        }
    }
    return -1;

}

void client_close(struct selector_key *key) {
    connection * client = (connection *) key->data;
    parser_destroy(client->parser);
    free(client->user_data.inbox.mails);
    if (client->user_data.inbox.rtrv_fd != -1)
    {
        selector_unregister_fd(key->s, client->user_data.inbox.rtrv_fd);
        close(client->user_data.inbox.rtrv_fd);
    }
    free(client);
    close(key->fd);
    // stats.concurrent_connections--;

    log(DEBUG, "%s", "Finished POP3 connection with a client");
}

void client_read(struct selector_key *key) {
    stm_handler_read(&((connection *)key->data)->stm, key);
}

void client_write(struct selector_key *key) {
    stm_handler_write(&((connection *)key->data)->stm, key);

    connection * client = (connection *) key->data;

    size_t size;
    char * ptr = (char *) buffer_read_ptr(&client->server_buffer, &size);
    ssize_t n = send(key->fd, ptr, size, 0);
    if (n == -1) {
        selector_unregister_fd(key->s, key->fd);
        return;
    }


    buffer_read_adv(&client->server_buffer, n);
    // stats.transferred_bytes += n;

    if (client->command.has_finished) {
        if (!client->active) {
            selector_unregister_fd(key->s, key->fd);
        } else if (buffer_can_read(&client->command_buffer)) {
            selector_set_interest_key(key, OP_NOOP);
            stm_handler_read(&((connection *) key->data)->stm, key);
        } else {
            selector_set_interest_key(key, OP_READ);
        }
    }
}


void accept_connection_handler(struct selector_key *key) {  

    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    const int client_fd = accept(key->fd, (struct sockaddr *)&client_addr, &client_addr_len);

    connection * client = calloc(1, sizeof(connection));

    if (client_fd == -1) {
        goto fail;
    }

    // Convierte en non blocking el socket nuevo.
    if (selector_fd_set_nio(client_fd) == -1) {
        goto fail;
    }

    log(INFO,"%s", "Initialized selector, will initialize STM.\n" )
    
    client->stm.states = stm_states_table;
    client->stm.initial = AUTHORIZATION;
    client->stm.max_state = STM_STATES_COUNT;
    stm_init(&client->stm);
    buffer_init(&client->command_buffer, MAX_BUFF_SIZE, (uint8_t *) client->command_buff);
    buffer_init(&client->server_buffer, MAX_BUFF_SIZE, (uint8_t *) client->server_buff);
    client->parser = parser_init(parser_no_classes(), &parser_deff);
    client->last_states = -1;
    client->user_data.inbox.rtrv_fd = -1;
    buffer_init(&client->command.retr_mail_buffer, MAX_BUFF_SIZE, (uint8_t *) client->command.mail_buffer);
    client->user_data.inbox.mails = calloc(args.max_mails, sizeof(mail));
    client->active = true;
    
    // Registrar el nuevo socket cliente con el selector.
    if (selector_register(key->s, client_fd, &client_handler, OP_READ, client) != SELECTOR_SUCCESS) {
        goto fail;
    }

    log(DEBUG,"%s","Established new POP3 connection");
    // stats.concurrent_connections++;
    // stats.historical_connections++;
    return;

fail:
    log(LOG_ERROR,"%s","FAIL! Could not accept connection\n");
    if (client_fd != -1) {
        free(client->user_data.inbox.mails);
        free(client);
        close(client_fd);
        log(LOG_ERROR,"%s","FREED resources\n");
    }
}

stm_states list(struct selector_key * key) {
    // if (!user->auth)
    // {
    //     send(user->socket_fd, "ERROR\n", 7, 0);
    //     return;
    // }else{
    //     for (int i = 0 ; i < user->inbox_size ; i++){

    //     }
    //     // open dir (maildir)
    //     send(user->socket_fd, "OK! \n",5,0);
    // }
    
 }

int store_connection(int socket_fd, connection * clients){
    int idx = get_user_buffer_idx(clients);
    if (idx == -1){
        //TODO: Logger
        log(LOG_ERROR,"%s","ERROR. No more connections are allowed right now. Try again later\n");
        return -1;
    }

    clients[idx].socket = socket_fd;
    clients[idx].active = true;
    log(INFO,"Estoy por hacer store del socket_fd: %d que se va a guardar en el client[%d]", clients[idx].socket, idx);

    return idx;

}

int get_user_buffer_idx(connection * clients){
    for (int i = 0 ; i < MAX_CLIENTS ; i++){
        if (!clients[i].active)
            return i;
    }
    return -1;
}


//--------------------WRITE AUTH COMMANDS-----------------------//

stm_states user_write(struct selector_key * key) {
    connection * client = (connection *) key->data;
    size_t size;
    char * str = (char *) buffer_write_ptr(&client->server_buffer,&size);
    char * message = "+OK";
    char * error_message = "-ERR Unknown User";
    size_t message_size = strlen(message);
    size_t error_message_size = strlen(error_message);
    
    if (client->command.has_error)
    {
        if (error_message_size > size - 2) { //-2 por el \n\r
            return AUTHORIZATION;
        }
        strncpy(str, error_message, error_message_size);
        strncpy(str + error_message_size, "\r\n", 2);
        size = error_message_size + 2;
    } else
    {
        if (message_size > size - 2) { //-2 por el \n\r
            return AUTHORIZATION;
        }
        strncpy(str, message, message_size);
        strncpy(str + message_size, "\r\n", 2);
        size = message_size + 2;
    }
    buffer_write_adv(&client->server_buffer,size);
    client->command.has_finished = true;
    return AUTHORIZATION;
}

stm_states pass_write(struct selector_key * key) {
    connection * client = (connection *) key->data;
    size_t size;
    char * str = (char *) buffer_write_ptr(&client->server_buffer,&size);
    char * message = "+OK Logged in.";
    char * error_message = "-ERR Invalid Password";
    size_t message_size = strlen(message);
    size_t error_message_size = strlen(error_message);
    
    if (client->command.has_error)
    {
        if (error_message_size > size - 2) { //-2 por el \n\r
            return AUTHORIZATION;
        }
        strncpy(str, error_message, error_message_size);
        strncpy(str + error_message_size, "\r\n", 2);
        size = error_message_size + 2;
    } else
    {
        if (message_size > size - 2) { //-2 por el \n\r
            return AUTHORIZATION;
        }
        strncpy(str, message, message_size);
        strncpy(str + message_size, "\r\n", 2);
        size = message_size + 2;
    }
    buffer_write_adv(&client->server_buffer,size);
    client->command.has_finished = true;
    return TRANSACTION;
}

stm_states capa_write(struct selector_key * key, stm_states state) {
    connection * client = (connection *) key->data;
    size_t size;
    char * str = (char *) buffer_write_ptr(&client->server_buffer,&size);
    char * message = "+OK\nUSER\nPIPELINING";
    size_t message_size = strlen(message);
    if (message_size > size - 2) { //-2 por el \n\r
        return state;
    }
    strncpy(str, message, message_size);
    strncpy(str + message_size, "\r\n", 2);
    size = message_size + 2; 
    buffer_write_adv(&client->server_buffer,size);
    client->command.has_finished = true;
    return state;
}

stm_states quit_writ(struct selector_key * key, stm_states state) {
    connection * client = (connection *) key->data;
    size_t size;
    char * str = (char *) buffer_write_ptr(&client->server_buffer,&size);
    char * message = "+OK Logging out.";
    size_t message_size = strlen(message);
    if (message_size > size - 2) { //-2 por el \n\r
        return state;
    }
    strncpy(str, message, message_size);
    strncpy(str + message_size, "\r\n", 2);
    size = message_size + 2; 
    buffer_write_adv(&client->server_buffer,size);
    client->command.has_finished = true;
    client->active = false;
    return QUIT;
}


//--------------WRITE TRAN COMMANDS-----------------//

stm_states list_write(struct selector_key * key){
    connection * client = (connection *) key->data;
    size_t size;
    char * str = (char *) buffer_write_ptr(&client->server_buffer,&size);
    
    char * error_message = "-ERR No such message";
    size_t error_message_length = strlen(error_message);

    if (client->command.args_index > 0) {
        char * end;
        unsigned long argument = strtol(client->command.args, &end, 10);
        if (end[0] != '\0' || argument - 1 >= client->user_data.inbox.dim) {
            if (error_message_length > size - 2) {
                return TRANSACTION;
            }
            strncpy(str, error_message, error_message_length);
            strncpy(str + error_message_length, "\r\n", 2);
            size = error_message_length + 2;
            buffer_write_adv(&client->server_buffer,size);
            client->command.has_finished = true;
            return TRANSACTION;
        }

        char message[MAX_BUFF_SIZE];
        size_t message_length = sprintf(message, "+OK %lu %u", argument, client->user_data.inbox.mails[argument - 1].size);

        if (message_length > size - 2) {
            return TRANSACTION;
        }
        strncpy(str, error_message, error_message_length);
        strncpy(str + error_message_length, "\r\n", 2);
        size = error_message_length + 2;
        buffer_write_adv(&client->server_buffer,size);
        client->command.has_finished = true;
        return TRANSACTION;
    }
    char message[MAX_BUFF_SIZE] = {0};
    size_t message_length = 0;
    if (!client->command.ok) {
        char * ok = "+OK\r\n";
        size_t ok_length = strlen(ok);
        if (ok_length > size) {
            return TRANSACTION;
        }
        strncpy(message, ok, ok_length);
        size -= ok_length;
        message_length += ok_length;
        client->command.ok = true;
    }
    while (client->user_data.inbox.idx < client->user_data.inbox.dim) {
        if (!client->user_data.inbox.mails[client->user_data.inbox.idx].to_delete) {
            char line[MAX_BUFF_SIZE];
            size_t line_length = sprintf(line, "%zu %u\r\n", client->user_data.inbox.idx + 1, client->user_data.inbox.mails[client->user_data.inbox.idx].size);
            if (line_length > size) {
                break;
            }
            strncpy(message + message_length, line, line_length);
            size -= line_length;
            message_length += line_length;
        }
        client->user_data.inbox.idx++;
    }
    if (client->user_data.inbox.idx == client->user_data.inbox.dim) {
        char * end = ".\r\n";
        size_t end_length = strlen(end);
        if (end_length <= size) {
            strncpy(message + message_length, end, end_length);
            size -= end_length;
            message_length += end_length;
            client->command.has_finished = true;
        }
    }

    strncpy(str, message, message_length);
    size = message_length;
    buffer_write_adv(&client->server_buffer,size);
    return TRANSACTION;
}

stm_states retr_write(struct selector_key * key){ 
    connection * client = (connection *) key->data;
    size_t size;
    char * str = (char *) buffer_write_ptr(&client->server_buffer,&size);
    
    char * error_message = "-ERR No such message";
    size_t error_message_length = strlen(error_message);

    if (client->command.has_error) {
        if (error_message_length > size - 2) {
            return TRANSACTION;
        }
        strncpy(str, error_message, error_message_length);
        strncpy(str + error_message_length, "\r\n", 2);
        size = error_message_length + 2;
        client->command.has_finished = true;
        buffer_write_adv(&client->server_buffer,size);
        return TRANSACTION;
    }

    if (!client->command.ok) {
        char * ok = "+OK message follows\r\n";
        size_t ok_length = strlen(ok);
        if (ok_length > size) {
            return TRANSACTION;
        }
        strncpy(str, ok, ok_length);
        size = ok_length;
        client->command.ok = true;
        buffer_write_adv(&client->server_buffer,size);
        selector_set_interest_key(key, OP_NOOP);
        selector_set_interest(key->s, client->user_data.inbox.rtrv_fd, OP_READ);
        return TRANSACTION;
    }

    while (buffer_can_write(&client->server_buffer)) {
        if (buffer_can_read(&client->command.retr_mail_buffer)) {
            char c;
            if (client->command.crlf == '.') {
                c = '.';
            } else {
                c = (char) buffer_read(&client->command.retr_mail_buffer);
            }
            if (c == '\r') {
                client->command.crlf = '\r';
            } else if (c == '\n' && client->command.crlf == '\r') {
                client->command.crlf = '\n';
            } else if (c == '.' && client->command.crlf == '\n') {
                buffer_write(&client->server_buffer, '.');
                if (!buffer_can_write(&client->server_buffer)) {
                    client->command.crlf = '.';
                    return TRANSACTION;
                }
            } else {
                client->command.crlf = 0;
            }
            buffer_write(&client->server_buffer, c);
        } else {
            if (client->user_data.inbox.rtrv_fd == -1) {
                char end[6];
                size_t end_length = 0;
                if (client->command.crlf != '\n') {
                    strcpy(end, "\r\n");
                    end_length = 2;
                }
                strcpy(end + end_length, ".\r\n");
                end_length += 3;

                size_t write_bytes;
                char * ptr = (char *) buffer_write_ptr(&client->server_buffer, &write_bytes);
                if (end_length > write_bytes) {
                    return TRANSACTION;
                }
                strncpy(ptr, end, end_length);
                buffer_write_adv(&client->server_buffer, end_length);

                client->command.has_finished = true;
                client->command.crlf = 0;
                return TRANSACTION;
            } else {
                break;
            }
        }
    }

    if (!buffer_can_read(&client->command.retr_mail_buffer)) {
        selector_set_interest_key(key, OP_NOOP);
        selector_set_interest(key->s, client->user_data.inbox.rtrv_fd, OP_READ);
    }
    return TRANSACTION;
}

stm_states dele_write(struct selector_key *key) {
    connection *client = (connection *)key->data;
    size_t size;
    char *str = (char *)buffer_write_ptr(&client->server_buffer, &size);
    char *error_message = "-ERR No such message\r\n";
    size_t error_message_length = strlen(error_message);

    char *end;
    unsigned long argument = strtol(client->command.args, &end, 10);
    if (end[0] != '\0' || argument - 1 >= client->user_data.inbox.dim || client->user_data.inbox.mails[argument - 1].to_delete) {
        if (error_message_length > size - 2) return TRANSACTION;
        strncpy(str, error_message, size = error_message_length + 2);
        buffer_write_adv(&client->server_buffer, size);
        client->command.has_finished = true;
        return TRANSACTION;
    }

    client->user_data.inbox.mails[argument - 1].to_delete = true;

    char *message = "+OK Message deleted\r\n";
    size_t message_length = strlen(message);
    if (message_length > size - 2) return TRANSACTION;
    strncpy(str, message, size = message_length + 2);
    client->user_data.inbox.byte_size -= client->user_data.inbox.mails[argument - 1].size;
    client->command.has_finished = true;
    buffer_write_adv(&client->server_buffer, size);
    return TRANSACTION;
}


stm_states noop_write(struct selector_key *key) {
    connection *client = (connection *)key->data;
    size_t size;
    char *str = (char *)buffer_write_ptr(&client->server_buffer, &size);
    char *message = "+OK\r\n";
    size_t message_size = strlen(message);

    if (message_size > size - 2) return TRANSACTION;
    size = message_size + 2;
    strncpy(str, message, size);
    buffer_write_adv(&client->server_buffer, size);
    client->command.has_finished = true;
    return TRANSACTION;
}


stm_states rset_write(struct selector_key * key){
    connection *client = (connection *)key->data;
    size_t size;
    char *str = (char *)buffer_write_ptr(&client->server_buffer, &size);
    char *message = "+OK\r\n";
    size_t message_size = strlen(message);

    if (message_size > size - 2) return TRANSACTION;
    size = message_size + 2;
    strncpy(str, message, size);
    buffer_write_adv(&client->server_buffer, size);
    client->command.has_finished = true;
    return TRANSACTION;
}

stm_states stat_write(struct selector_key * key){
    connection *client = (connection *)key->data;
    size_t size;
    char *str = (char *)buffer_write_ptr(&client->server_buffer, &size);

    char message[MAX_BUFF_SIZE];

    size_t mail_count = 0;
    for (size_t i = 0; i < client->user_data.inbox.dim; i++) {
        if (client->user_data.inbox.mails[i].to_delete) {
            mail_count++;
        }
    }

    size_t message_size = sprintf(message, "+OK %zu %zu", mail_count, client->user_data.inbox.byte_size);

    if (message_size > size - 2) {
        return TRANSACTION;
    }

    if (message_size > size - 2) return TRANSACTION;
    strncpy(str, message, message_size);
    strncpy(str + message_size, "\r\n", 2);
    size = message_size + 2;
    buffer_write_adv(&client->server_buffer, size);
    client->command.has_finished = true;
    return TRANSACTION;
}


// -------- AUTH COMMANDS  -------------------//

stm_states user(struct selector_key * key)
{
    connection * client = (connection *) key->data;
    char * requested_username = client->command.args;

    // check given user arg ==  registered user
    for ( size_t i = 0 ; i < args.user_count ; i++ )
    {
        if (strcmp(args.users[i].name, requested_username) == 0)
        { // valid username
            // set requested username in client user data
            client->user_data.requested = true;
            client->user_data.username_index = i;
            strcpy(client->user_data.username, args.users[i].name);
            return AUTHORIZATION;
        }
    }

    client->user_data.requested = false;
    client->user_data.auth = false;
    client->user_data.username_index = -1;
    client->user_data.username[0] = 0;
    return AUTHORIZATION;
}

stm_states pass(struct selector_key * key)
{
    connection * client = (connection *) key->data;
    char * requested_pass = client->command.args;
    
    if ( client->user_data.requested == true )
    { // user was requested
        int server_idx = client->user_data.username_index;
        // check password
        if (strcmp(args.users[server_idx].pass, requested_pass) == 0)
        { // correct password
            client->user_data.auth = true;
            return TRANSACTION;
        }
    }

    return AUTHORIZATION;

}

// ------------------ BOTH STATES ---------------------------- //

stm_states capa(struct selector_key * key)//Revisar!!!!!
{
    
    return AUTHORIZATION;  
}

stm_states quit(struct selector_key * key)//Revisar!!!!!
{
    return QUIT;
}


// -------- TRAN COMMANDS  -------------------//

stm_states list(struct selector_key * key)//Revisar!!!!!
{
    connection * client = (connection *) key->data;

    client->command.has_finished = false;

    // mainly a write OP
    return TRANSACTION;
}

void read_mail(struct selector_key * key) {
    connection * client = (connection *) key->data;

    size_t write_bytes;
    char * ptr = (char *) buffer_write_ptr(&client->command.retr_mail_buffer, &write_bytes);
    ssize_t n = read(client->user_data.inbox.rtrv_fd, ptr, write_bytes);
    buffer_write_adv(&client->command.retr_mail_buffer, n);
    
    if (n == 0) {
        selector_unregister_fd(key->s, client->user_data.inbox.rtrv_fd);
        close(client->user_data.inbox.rtrv_fd);
        client->user_data.inbox.rtrv_fd = -1;
    } else  {
        selector_set_interest_key(key, OP_NOOP);
    }
    selector_set_interest(key->s, key->fd, OP_WRITE);
}

// only needed for selector function transition
// specific to retr function
struct fd_handler selector_read_mail = {
        .handle_read = read_mail
};

stm_states retr(struct selector_key * key)
{
    connection * client = (connection *) key->data;

    client->user_data.inbox.rtrv_fd = -1;

    // atoi returns 0 on a non-number string
    // our args ends in \0
    size_t requested = atoi(client->command.args);

    size_t idx = requested - 1;

    if ( requested > 0 && requested <= client->user_data.inbox.dim && client->user_data.inbox.mails[idx].to_delete == false)
    { // valid requested mail
        int fd = open(client->user_data.inbox.mails[idx].path);
        client->user_data.inbox.rtrv_fd = fd;

        if (fd == -1)
        {
            client->command.has_error = true;
            return TRANSACTION;
        }

        if (selector_register(key->s, fd, &selector_read_mail, OP_NOOP, client) != SELECTOR_SUCCESS)
        {
            close(fd);
            client->command.has_error = true;
            return TRANSACTION;
        }

    }

    client->command.has_error = true;
    return TRANSACTION;


}

stm_states dele(struct selector_key * key)
{
    
}

stm_states rset(struct selector_key * key)
{
    
}

stm_states noop(struct selector_key * key)
{
    
}

stm_states pop_stat(struct selector_key * key)
{
    
}