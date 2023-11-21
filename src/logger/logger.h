#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>

typedef enum {DEBUG=0, INFO, LOG_ERROR, FATAL} LOG_LEVEL;

extern LOG_LEVEL current_level;

/**
*  Minimo nivel de log a registrar. Cualquier llamada a log con un nivel mayor a newLevel sera ignorada
**/
void setLogLevel(LOG_LEVEL newLevel);

char * levelDescription(LOG_LEVEL level);

// Debe ser una macro para poder obtener nombre y linea de archivo. 
#define log(level, fmt, ...)   {if(level >= current_level) {\
	fprintf (stderr, "%s: %s:%d, ", levelDescription(level), __FILE__, __LINE__); \
	fprintf(stderr, fmt, ##__VA_ARGS__); \
	fprintf(stderr,"\n"); }\
	if ( level==FATAL) exit(1);}
#endif