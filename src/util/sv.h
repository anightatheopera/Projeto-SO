#ifndef SV_H
#define SV_H

#include <stddef.h>

typedef struct {
    const char* data;
    size_t count;
} SV;

SV sv_slurp_file(const char* filename);

SV sv_from_cstr(const char* cstr);

ssize_t sv_write(SV sv, int fd);

#endif