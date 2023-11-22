#ifndef POP_UTILS_H_
#define POP_UTILS_H_

#include "../parser/parser.h"
#include "pop3.h"

#define MAX_COMMAND_LENGTH 4




// command state
void parser_command_space(struct parser_event * ret, uint8_t c, void * data);
void parser_command_return(struct parser_event * ret, uint8_t c, void * data);
void parser_command_any(struct parser_event * ret, uint8_t c, void * data);

// argument state
void parser_argument_any(struct parser_event * ret, uint8_t c, void * data);
void parser_argument_return(struct parser_event * ret, uint8_t c, void * data);

//end state
void parser_end_enter(struct parser_event * ret, uint8_t c, void * data);
void parser_end_any(struct parser_event * ret, uint8_t c, void * data);

#endif