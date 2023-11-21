#include "pop_utils.h"

void authorization_arrival(stm_states state, struct selector_key * key){}
void authorization_departure(stm_states state, struct selector_key * key){}
stm_states authorization_read(struct selector_key * key){
    return 0;
}
stm_states authorization_write(struct selector_key * key){
        return 0;

}

void transaction_arrival(stm_states state, struct selector_key * key){}
void transaction_departure(stm_states state, struct selector_key * key){}
stm_states transaction_read(struct selector_key * key){
        return 0;

}
stm_states transaction_write(struct selector_key * key){
        return 0;

}

void error_arrival(stm_states state, struct selector_key * key){}
void error_departure(stm_states state, struct selector_key * key){}
stm_states error_read(struct selector_key * key){
        return 0;

}
stm_states error_write(struct selector_key * key){
        return 0;

}

void quit_arrival(stm_states state, struct selector_key * key){}
void quit_departure(stm_states state, struct selector_key * key){}
stm_states quit_read(struct selector_key * key){    
    return 0;
}
stm_states quit_write(struct selector_key * key){    
    return 0;
}