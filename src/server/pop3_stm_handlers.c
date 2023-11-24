#include "pop3_stm_handlers.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>


// ---- SERVER ARGS ----- //
extern struct popargs args;


// ES MUY IMPORTANTE QUE LOS COMANDOS ESTEN EN EL MISMO ORDEN QUE EL ENUM 
char * auth_state_commands[] = {"USER", "PASS", "CAPA", "QUIT"};
size_t auth_commands_dim = 4;

char * trans_state_commands[] = {"STAT", "LIST", "RETR", "DELE", "RSET", "NOOP", "CAPA", "QUIT"};
size_t trans_commands_dim = 8;

stm_states read_command(struct selector_key * key, stm_states current_state) {
    connection * cliente = (connection*) key->data;
    log(INFO,"%s","read_command");

    if (!buffer_can_read(&cliente->command_buffer))
    {
        log(INFO,"%s","must write on command buffer");
        size_t write_dim;
        char * write_ptr = (char *) buffer_write_ptr(&cliente->command_buffer, &write_dim);
        // write on buffer
        ssize_t n = recv(key->fd, write_ptr, write_dim, 0);
        if (n == 0) return QUIT;
        // advance write ptr
        buffer_write_adv(&cliente->command_buffer, n);
    } // if cant read then write
    
    log(DEBUG, "%s", "command buffer not blocked");

    // read buffer
    size_t to_read = 0;
    char * ptr = (char *) buffer_read_ptr(&cliente->command_buffer, &to_read);
    
    for ( size_t i = 0 ; i < to_read; i++)
    { // parse buffer
        // parse one char at a time
        const struct parser_event * event = parser_feed(cliente->parser, ptr[i], key->data);
        buffer_read_adv(&cliente->command_buffer, 1);

        if (event->type == VALID)
        {
            log(DEBUG, "Current State: %u. Authorization = %u", cliente->stm.current->state, AUTHORIZATION);
            if ((stm_states) cliente->stm.current->state == AUTHORIZATION)
            {
                log(DEBUG, "Checking for a valid AUTH command: %s ", cliente->command.command)
                for(size_t i = 0 ; i < auth_commands_dim; i++)
                {
                    if (strcasecmp(auth_state_commands[i], cliente->command.command) == 0)
                    { // valid authorization command
                        stm_states next = execute_auth_command(i, key);
                        selector_set_interest_key(key,OP_WRITE);
                        return next;
                    }                    
                }
            }else if ( (stm_states) cliente->stm.current->state == TRANSACTION)
            {
                for (size_t i = 0 ; i < trans_commands_dim; i++)
                {
                    log(DEBUG, "Checking for a valid TRANS command: %s ", cliente->command.command)
                    if (strcasecmp(trans_state_commands[i], cliente->command.command) == 0)
                    { // valid transactional command
                        stm_states next = execute_trans_command(i, key);
                        selector_set_interest_key(key,OP_WRITE);
                        return next;

                    }
                }
            }else
            {
                return ERROR;
            }
        }
        else if (event->type == PARSE_ERROR)
        {
            // while (i < to_read) {
            //     bool saw_carriage_return = ptr[i] == '\r';
            //     char c = (char) buffer_read(&cliente->command_buffer);
            //     if (c == '\r') {
            //         saw_carriage_return = true;
            //     } else if (c == '\n') {
            //         if (saw_carriage_return) {
            //             return ERROR;
            //         }
            //     } else {
            //         saw_carriage_return = false;
            //     }
            //     i++;
            // }
            // return ERROR;
        }
        
    }
    
    return current_state;

}

// Requests to Server by user

stm_states execute_auth_command(auth_commands command, struct selector_key * key)
{
    log(DEBUG, "%s", "auth command dispatcher");
    switch(command)
    {
        case USER:
            return user(key);
            break;
        case PASS:
            return pass(key);
            break;
        case AU_CAPA:
            return auth_capa(key);
            break;
        case AU_QUIT:
            return auth_quit(key);
            break;
        default:
            return AUTHORIZATION;
    }

}

stm_states execute_trans_command(trans_commands command, struct selector_key * key)
{
    log(DEBUG, "%s", "trans command dispatcher");
    switch(command)
    {
        case STAT:
            return pop_stat(key);
            break;
        case LIST:
            return list(key);
            break;
        case RETR:
            return retr(key);
            break;
        case DELE:
            return dele(key);
            break;
        case RSET:
            return rset(key);
            break;
        case NOOP:
            return noop(key);
            break;
        case TR_CAPA:
            return trans_capa(key);
            break;
        case TR_QUIT:
            return trans_quit(key); 
            break;
        default:
            return TRANSACTION;
    }
}

// End of RtSbU

stm_states write_command(struct selector_key * key, stm_states current_state) {
    
    connection * client = (connection *) key->data;

    // log(DEBUG, "%s", "Entered write command");
    if (buffer_can_write(&client->server_buffer))
    {
        if (current_state == AUTHORIZATION)
        {
            log(DEBUG, "Entered AUTH write command : -- %s --", client->command.command);
            for (size_t i = 0; i < auth_commands_dim; i++)
            {
                char * current = auth_state_commands[i];
                if (strcasecmp(current, client->command.command) == 0)
                {
                    log(DEBUG, "Valid AUTH command: -- %s --", client->command.command);
                    stm_states next = auth_writer(key,i);
                    if (client->command.has_finished) {
                        client->command.args[0] = 0;
                        client->command.args_index = 0;
                        client->command.command[0] = 0;
                        client->command.command_index = 0;
                    }
                    return next;
                }
            }
        }
        else if (current_state == TRANSACTION)
        {
            log(DEBUG, "Entered TRANS write command : -- %s --", client->command.command);
            for (size_t i = 0; i < trans_commands_dim; i++)
            {
                char * current = trans_state_commands[i];
                if (strcasecmp(current, client->command.command) == 0)
                {
                    log(DEBUG, "Valid TRANS command: -- %s --", client->command.command);
                    stm_states next = trans_writer(key,i);
                    if (client->command.has_finished) {
                        client->command.args[0] = 0;
                        client->command.args_index = 0;
                        client->command.command[0] = 0;
                        client->command.command_index = 0;
                    }
                    return next;
                }
            }
        }        
    }
    return current_state;
}

//Response from Server

stm_states auth_writer(struct selector_key * key, auth_commands command){
    switch (command)
    {
    case USER:
        return user_write(key);
        break;
    case PASS:
        return pass_write(key);
        break;
    case AU_CAPA:
        return capa_write(key, AUTHORIZATION);
        break;
    case QUIT:
        return quit_writ(key, AUTHORIZATION);
        break;
    default:
        return AUTHORIZATION;
        break;
    }
}

stm_states trans_writer(struct selector_key * key, trans_commands command){
    
    switch(command)
    {
        case STAT:
            stat_write(key);
            break;
        case LIST:
            list_write(key);
            break;
        case RETR:
            retr_write(key);
            break;
        case DELE:
            dele_write(key);
            break;
        case RSET:
            rset_write(key);
            break;
        case NOOP:
            noop_write(key);
            break;
        case TR_CAPA:
            capa_write(key, TRANSACTION);
            break;
        case TR_QUIT:
            quit_writ(key, TRANSACTION);
            break;
    }

    return TRANSACTION;
}

//End of RfS.

void authorization_arrival(stm_states state, struct selector_key * key){
    log(INFO,"%s","authorization_arrival");
}
void authorization_departure(stm_states state, struct selector_key * key){
    log(INFO,"%s","authorization_departure");
    connection * client = (connection *) key->data;
    client->last_states = state;
}
stm_states authorization_read(struct selector_key * key){
    log(INFO,"%s","authorization_read");
    return read_command(key, AUTHORIZATION);
}
stm_states authorization_write(struct selector_key * key){
    log(INFO, "%s", "authorization_write");
    connection * client = (connection *) key->data;
    if ((int)client->last_states == -1)
    {
        log(DEBUG, "%s", "GREET!");
        char * message = "+OK POP3 server ready\r\n";
        size_t write_bytes;
        char * ptr = (char *) buffer_write_ptr(&client->server_buffer, &write_bytes);
        if (write_bytes >= strlen(message)) {
            client->command.has_finished = true;
            strncpy(ptr, message, strlen(message));
            buffer_write_adv(&client->server_buffer, (ssize_t) strlen(message));
            client->last_states = AUTHORIZATION;
        }    
    }
    return write_command(key, AUTHORIZATION);
}

void transaction_arrival(stm_states state, struct selector_key * key){

    connection * client = (connection *) key->data;
// /mails/facha
    // must reconnect to update mails
    if (client->user_data.inbox.dim != 0) return;

    DIR * dir = opendir(client->user_data.inbox.mail_dir);
    struct dirent * file;

    log(DEBUG, "El mail_dir: %s  -- del user: %s  -- exito: %d ", client->user_data.inbox.mail_dir, client->user_data.username, dir != NULL);

    while (client->user_data.inbox.dim < args.max_mails && (file = readdir(dir)) != NULL)
    {
        if (strcmp(".", file->d_name) == 0 || strcmp("..", file->d_name) == 0) continue;
        
        size_t i = client->user_data.inbox.dim;

        strcat(client->user_data.inbox.mails[i].path, client->user_data.inbox.mail_dir);
        strcat(client->user_data.inbox.mails[i].path, "/");
        strcat(client->user_data.inbox.mails[i].path, file->d_name);

        struct stat stat_data;
        if (stat(client->user_data.inbox.mails[i].path, &stat_data) == 0) {
            client->user_data.inbox.mails[i].size = stat_data.st_size;
            client->user_data.inbox.byte_size += stat_data.st_size;
            client->user_data.inbox.dim++;
        } else {
            client->user_data.inbox.mails[i].path[0] = '\0';
        }
    }

    closedir(dir);

    log(DEBUG, "%s", "transaction_arrival success");
}
void transaction_departure(stm_states state, struct selector_key * key){
    log(INFO, "%s", "transaction_departure");
    connection * client = (connection *) key->data;
    client->last_states = state;
}
stm_states transaction_read(struct selector_key * key){
    log(INFO, "%s", "transaction_read");
    return read_command(key,TRANSACTION);

}
stm_states transaction_write(struct selector_key * key){
    log(INFO, "%s", "transaction_write");
    return write_command(key, TRANSACTION);

}

void error_arrival(stm_states state, struct selector_key * key){
    log(INFO, "%s", "error_arrival");
    connection * client = (connection *) key->data;
    client->command.command[0] = '\0';
    client->command.command_index = 0;
    client->command.args[0] = '\0';
    parser_reset(client->parser);
    selector_set_interest_key(key, OP_WRITE);
}
void error_departure(stm_states state, struct selector_key * key){
    log(INFO, "%s", "error_departure");
    connection * client = (connection *) key->data;
    client->last_states = state;
}
stm_states error_read(struct selector_key * key){
    log(INFO, "%s", "error_read");
    abort();
}
stm_states error_write(struct selector_key * key){
    connection * client = (connection *) key->data;
    char * message = "-ERR Invalid Command\r\n";
    size_t write_bytes;
    char * ptr = (char *) buffer_write_ptr(&client->server_buffer, &write_bytes);
    if (write_bytes >= strlen(message)) {
        client->command.has_finished = true;
        strncpy(ptr, message, strlen(message));
        buffer_write_adv(&client->server_buffer, (ssize_t) strlen(message));
        return client->last_states;
    }    
    return ERROR;
}

void quit_arrival(stm_states state, struct selector_key * key){
    log(INFO, "%s", "quit_arrival");
    connection * client = (connection *) key->data;
    if (client->active) {
        log(DEBUG,"FD %d: Abort received", key->fd);
        selector_unregister_fd(key->s, key->fd);
    }
}
void quit_departure(stm_states state, struct selector_key * key){
    log(INFO, "%s", "quit_departure");
    abort();
}
stm_states quit_read(struct selector_key * key){
    log(INFO, "%s", "quit_read");    
    abort();
}
stm_states quit_write(struct selector_key * key){
    log(INFO, "%s", "quit_write");    
    abort();
}