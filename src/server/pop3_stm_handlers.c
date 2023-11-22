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
                    { // valid command
                        execute_auth_command(i, key);
                    }
                }
            }else if (cliente->stm.current == TRANSACTION)
            {
                if (strcasecmp(trans_state_commands[i], cliente->command.command) == 0)
                {
                    execute_trans_command(i, key);
                }
                
            }else
            {
                return ERROR;
            }
            strcasecmp();
        }
    }
    
    selector_set_interest_key(key,OP_READ);
    return current_state;

}

int execute_auth_command(auth_commands command, struct selector_key * key)
{
    switch(command)
    {
        case USER:
            // do stuff
            break;
        case PASS:
            break;
        case AU_CAPA:
            break;
        case AU_QUIT:
            break;
    }

    return 1;
}

int execute_trans_command(trans_commands command, struct selector_key * key)
{
    switch(command)
    {
        case STAT:
            // do stuff
            break;
        case LIST:
            break;
        case RETR:
            break;
        case DELE:
            break;
        case RSET:
            break;
        case NOOP:
            break;
        case TR_CAPA:
            break;
        case TR_QUIT:
            break;
    }

    return 1;
}

stm_states write_command(struct selector_key * key, stm_states current_state) {
    connection * client = (connection *) key->data;
    char * ptr;

    if (buffer_can_write(&client->server_buff))
    {
        size_t size;
        ptr = (char *) buffer_write_ptr(&client->server_buff,&size);
        if (current_state == AUTHORIZATION)
        {
            for (size_t i = 0; i < auth_commands_dim; i++)
            {
                char * current = auth_state_commands[i];
                if (strcasecmp(current, client->command.command) == 0)
                {
                    stm_states next = auth_writer(key,i,ptr, &size);
                    buffer_write_adv(&client->server_buff,size);
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
                    stm_states next = trans_writer(key, ptr, &size);
                    buffer_write_adv(&client->server_buff,size);
                    // TODO: que hacer al terminar de ejecutar un comando
                    return next;
                }
            }
        }        
    }
    return current_state;
}

stm_states auth_writer(struct selector_key * key, auth_commands command,char * str, size_t * size){
    switch (command)
    {
    case USER:
        user_write(key, str, size);
        break;
    case PASS:
        break;
    case AU_CAPA:
        break;
    case QUIT:
        break;
    default:
        break;
    }
}

void authorization_arrival(stm_states state, struct selector_key * key){
    log(INFO,"%s","authorization_arrival");
}
void authorization_departure(stm_states state, struct selector_key * key){
    log(INFO,"%s","authorization_departure");
}
stm_states authorization_read(struct selector_key * key){
    log(INFO,"%s","authorization_read");
    return read_command(key, AUTHORIZATION);
}
stm_states authorization_write(struct selector_key * key){
    log(INFO, "%s", "authorization_write");
    return write_command(key, AUTHORIZATION);

}

void transaction_arrival(stm_states state, struct selector_key * key){
    log(INFO, "%s", "transaction_arrival");
}
void transaction_departure(stm_states state, struct selector_key * key){
    log(INFO, "%s", "transaction_departure");
}
stm_states transaction_read(struct selector_key * key){
    log(INFO, "%s", "transaction_read");
        return 0;

}
stm_states transaction_write(struct selector_key * key){
    log(INFO, "%s", "transaction_write");
    return write_command(key, TRANSACTION);

}

void error_arrival(stm_states state, struct selector_key * key){
    log(INFO, "%s", "error_arrival");
}
void error_departure(stm_states state, struct selector_key * key){
    log(INFO, "%s", "error_departure");
}
stm_states error_read(struct selector_key * key){
    log(INFO, "%s", "error_read");
        return 0;

}
stm_states error_write(struct selector_key * key){
    log(INFO, "%s", "error_write");
    return write_command(key, ERROR);

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