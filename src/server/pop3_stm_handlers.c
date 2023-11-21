#include "pop3_stm_handlers.h"

stm_states read_command(struct selector_key * key, stm_states current_state) {
    connection * cliente = (connection*) key->data;
    log(INFO,"%s","read_command");
    size_t to_read = 0;
    char * ptr = buffer_read_ptr(&cliente->command_buffer, &to_read);
        // ptr = (char *) buffer_read_ptr(&connection->in_buffer_object, &read_bytes);

    // const struct parser_event * event = parser_feed(connection->parser, ptr[i], connection);
    for ( size_t i = 0 ; i < to_read; i++)
    {
        const struct parser_event * event = parser_feed(cliente->parser, ptr[i]);
    }
    
    selector_set_interest_key(key,OP_READ);
    return current_state;
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
    return 0;

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
        return 0;

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
        return 0;

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