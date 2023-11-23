#include "pop_utils.h"
#include <stdio.h>

//#include <parser.h>

// command state
void parser_command_space(struct parser_event * ret, uint8_t c, void * data)
{
    connection * client = (connection *) data;
    int current_index = client->command.command_index;
    if ( current_index + 1 < MAX_COMMAND_LENGTH)
    {
        ret->type = PARSE_ERROR;
        return;   
    }
    ret->type = UNDEFINED;
    client->command.command[current_index] = 0;
}

void parser_command_return(struct parser_event * ret, uint8_t c, void * data)
{
    connection * client = (connection *) data;
    int current_index = client->command.command_index;
    if ( current_index + 1 < MAX_COMMAND_LENGTH)
    {
        ret->type = PARSE_ERROR;
        return;   
    }
    ret->type = UNDEFINED;
    client->command.command[current_index] = 0;
}
void parser_command_any(struct parser_event * ret, uint8_t c, void * data)
{
    connection * client = (connection *) data;
    int current_index = client->command.command_index;
    if ( current_index + 1 > MAX_COMMAND_LENGTH)
    {
        ret->type = PARSE_ERROR;
        return;   
    }
    ret->type = UNDEFINED;
    client->command.command[current_index] = (char) c;
    client->command.command_index++;
}

// argument state
void parser_argument_any(struct parser_event * ret, uint8_t c, void * data)
{
    connection * client = (connection *) data;
    int current_index = client->command.args_index;
    if ( current_index + 1 > MAX_ARGS_LENGTH)
    {
        ret->type = PARSE_ERROR;
        return;   
    }
    ret->type = UNDEFINED;
    client->command.args[current_index] = (char) c;
    client->command.args_index++;
}
void parser_argument_return(struct parser_event * ret, uint8_t c, void * data)
{
    connection * client = (connection *) data;
    int current_index = client->command.args_index;
    if ( current_index == 0 )
    {
        ret->type = PARSE_ERROR;
        return;   
    }
    ret->type = UNDEFINED;
    client->command.args[current_index] = 0;
}

//end state
void parser_end_enter(struct parser_event * ret, uint8_t c, void * data)
{
    ret->type = VALID;
}

void parser_end_any(struct parser_event * ret, uint8_t c, void * data)
{
    ret->type = PARSE_ERROR;
}