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

SV sv_chop_line(SV* sv);

SV sv_chop_word(SV* sv);

void sv_trim_whitespace(SV* sv);

long sv_to_long(SV sv);

char* sv_dup(SV sv);

#endif