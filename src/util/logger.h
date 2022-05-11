#ifndef LOGGER_H
#define LOGGER_H

#include <unistd.h>

#define logger_log(fmt) logger_default("[LOG][%d] " fmt "\n", getpid())
#define logger_log_fmt(fmt, ...) logger_default("[LOG][%d] " fmt "\n", getpid(), __VA_ARGS__)


#ifdef LOGGER_DEBUG
#define logger_debug(fmt) logger_default("[DEBUG][%d] " fmt "\n", getpid())
#define logger_debug_fmt(fmt, ...) logger_default("[DEBUG][%d] " fmt "\n", getpid(), __VA_ARGS__)
#else
#define logger_debug(fmt)
#define logger_debug_fmt(fmt, ...)
#endif


/*
    format: meter a string como se mete no printf
    n: numero de argumentos extra passados à funçao
    ...: quantidade variavel de argumentos
*/
void logger_default(const char *restrict fmt, ...);

#endif