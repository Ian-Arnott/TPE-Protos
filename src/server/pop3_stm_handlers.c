#include "pop3_stm_handlers.h"


// ES MUY IMPORTANTE QUE LOS COMANDOS ESTEN EN EL MISMO ORDEN QUE EL ENUM 
char ** auth_state_commands = {"USER", "PASS", "CAPA", "QUIT"};
size_t auth_commands_dim = sizeof(auth_state_commands);

char ** trans_state_commands = {"STAT", "LIST", "RETR", "DELE", "RSET", "NOOP", "CAPA", "QUIT"};
size_t trans_commands_dim = sizeof(trans_state_commands);

stm_states read_command(struct selector_key * key, stm_states current_state) {
    connection * cliente = (connection*) key->data;
    log(INFO,"%s","read_command");

    if (!buffer_can_read(&cliente->command_buffer))
    {
        size_t write_dim;
        char * write_ptr = (char *) buffer_write_ptr(&cliente->command_buffer, &write_dim);
        // write on buffer
        ssize_t n = recv(key->fd, write_ptr, write_dim, 0);
        if (n == 0) return QUIT;
        // advance write ptr
        buffer_write_adv(&cliente->command_buffer, n);
    } // if cant read then write
    

    // read buffer
    size_t to_read = 0;
    char * ptr = buffer_read_ptr(&cliente->command_buffer, &to_read);
    
    for ( size_t i = 0 ; i < to_read; i++)
    { // parse buffer
        // parse one char at a time
        struct parser_event * event = parser_feed(cliente->parser, ptr[i], key->data);
        buffer_read_adv(&cliente->command_buffer, 1);

        if (event->type == VALID)
        {
            if (cliente->stm.current == AUTHORIZATION)
            {
                for(int i = 0 ; i < auth_commands_dim ; i++)
                {
                    if (strcasecmp(auth_state_commands[i], cliente->command.command) == 0)
                    { // valid authorization command
                        stm_states next = execute_auth_command(i, key);
                        //interest
                        return next;
                    }
                }
            }else if (cliente->stm.current == TRANSACTION)
            {
                for (int i = 0 ; i < trans_commands_dim ; i++)
                {
                    if (strcasecmp(trans_state_commands[i], cliente->command.command) == 0)
                    { // valid transactional command
                        stm_states next = execute_trans_command(i, key);
                        // selector_set_interest_key(key,OP_READ); FIND OUT WHICH INTEREST
                        return next;

                    }
                }
                
                
            }else
            {
                return ERROR;
            }
            strcasecmp();
        }
    }
    
    return current_state;

}

// Requests to Server by user

stm_states execute_auth_command(auth_commands command, struct selector_key * key)
{
    switch(command)
    {
        case USER:
            return user(key);
            // do stuff
            // check length / user exists / etc
            // update key with authenticated connection data
            // command.hasError = false
            // return authorization state // aun falta PASS
            // transitions come from parser
            // call write_command() to give client info
            break;
        case PASS:
            return pass(key);
            break;
        case AU_CAPA:
            return capa(key);
            break;
        case AU_QUIT:
            return quit(key);
            break;
    }

    return 1;
}

int execute_trans_command(trans_commands command, struct selector_key * key)
{
    switch(command)
    {
        case STAT:
            stat();
            break;
        case LIST:
            list(key);
            break;
        case RETR:
            retr(key);
            break;
        case DELE:
            dele(key);
            break;
        case RSET:
            rset(key);
            break;
        case NOOP:
            noop();
            break;
        case TR_CAPA:
            capa(key);
            break;
        case TR_QUIT:
            break;
    }

    return 1;
}

// End of RtSbU

stm_states write_command(struct selector_key * key, stm_states current_state) {
    connection * client = (connection *) key->data;
    char * ptr;

    if (buffer_can_write(&client->server_buff))
    {

        if (current_state == AUTHORIZATION)
        {
            for (size_t i = 0; i < auth_commands_dim; i++)
            {
                char * current = auth_state_commands[i];
                if (strcasecmp(current, client->command.command) == 0)
                {
                    stm_states next = auth_writer(key,i);
                    // TODO: que hacer al terminar de ejecutar un comando
                    return next;
                }
            }
        }
        else if (current_state == TRANSACTION)
        {
            for (size_t i = 0; i < trans_commands_dim; i++)
            {
                char * current = trans_state_commands[i];
                if (strcasecmp(current, client->command.command) == 0)
                {
                    stm_states next = trans_writer(key,i);
                    // TODO: que hacer al terminar de ejecutar un comando
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
        break;
    }
}

stm_states trans_writer(struct selector_key * key, trans_commands command){
    
    switch(command)
    {
        case STAT:
            stat_write();
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
            noop_write();
            break;
        case TR_CAPA:
            capa_write(key, TRANSACTION);
            break;
        case TR_QUIT:
            quit_writ(key, TRANSACTION);
            break;
    }

    return 1;
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
    connection * client = key->data;
    if (client->last_states == -1)
    {
        char * message = "+OK POP3 server ready\r\n";
        size_t write_bytes;
        char * ptr = (char *) buffer_write_ptr(&client->server_buff, &write_bytes);
        if (write_bytes >= strlen(message)) {
            client->command.has_finished = true;
            strncpy(ptr, message, strlen(message));
            buffer_write_adv(&client->server_buff, (ssize_t) strlen(message));
            client->last_states = AUTHORIZATION;
        }    
        return AUTHORIZATION;
    }
    return write_command(key, AUTHORIZATION);
}

void transaction_arrival(stm_states state, struct selector_key * key){
    log(INFO, "%s", "transaction_arrival");
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
    char * ptr = (char *) buffer_write_ptr(&client->server_buff, &write_bytes);
    if (write_bytes >= strlen(message)) {
        client->command.has_finished = true;
        strncpy(ptr, message, strlen(message));
        buffer_write_adv(&client->server_buff, (ssize_t) strlen(message));
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