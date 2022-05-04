#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>

#define LOGGER_BUFFER_SIZE 1024
static char logger_buffer[LOGGER_BUFFER_SIZE];

/*
    imprime uma string formatada para o STDOUT (semelhante ao 'printf')
*/
void logger_default(const char *restrict format, ...){
	va_list ap;
	va_start(ap, format);

    int sz = vsprintf(logger_buffer, format, ap);
    if(sz > 0){
        write(STDOUT_FILENO, logger_buffer, (size_t) sz);
    }

    va_end(ap);
}
