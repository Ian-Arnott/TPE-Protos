#include "pop_utils.h"
#include <stdio.h>

//#include <parser.h>

// command state
void parser_command_space(struct parser_event * ret, uint8_t c, void * data)
{
    // connection conn = (connection) data;

    // if(conn->current_comando.comando_len + 1 > COMMAND_LEN ){
    //     ret->type = INVALID_COMMAND;
    //     return;
    // }

    // ret->type = UNDEFINED;
    // conn->current_comando.comando[conn->current_comando.comando_len] = '\0';
    // conn->current_comando.comando_len++;
}

void parser_command_return(struct parser_event * ret, uint8_t c, void * data)
{
    
}
void parser_command_any(struct parser_event * ret, uint8_t c, void * data)
{
    
}

// argument state
void parser_argument_any(struct parser_event * ret, uint8_t c, void * data)
{

}
void parser_argument_return(struct parser_event * ret, uint8_t c, void * data)
{
    
}

//end state
void parser_end_enter(struct parser_event * ret, uint8_t c, void * data)
{
    ret->type = VALID_COMMAND;
}

void parser_end_any(struct parser_event * ret, uint8_t c, void * data)
{
    ret->type = INVALID_COMMAND;
}