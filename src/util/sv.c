#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

#include "sv.h"

/* Return a String View with no contents */
SV sv_empty(){
    return (SV) {
        .data = NULL,
        .count = 0
    };
}

/* Load entire file from memory and return it in a String View */
SV sv_slurp_file(const char* filename){
    /* Open the file */
    int fd = open(filename, O_RDONLY);
    if(fd < 0){
        goto err_open;
    }

    /* Get size of the file */
    struct stat s;
    int status = fstat(fd, &s);
    size_t size = (size_t) s.st_size;
    if(status < 0){
        goto err_stat;
    }
    
    /* Allocate memory necessary to hold contents of the file */
    char* data = (char*) malloc(size * sizeof(char));
    if(data == NULL){
        goto err_malloc;
    }

    /* Read the file to data */
    ssize_t rd = read(fd, data, size);
    if(rd != (ssize_t) size){
        goto err_read;
    }

    close(fd);

    return (SV) {
        .data = data,
        .count = size
    };

err_read:
err_malloc:
    free(data);
err_stat:
    close(fd);
err_open:
    return sv_empty();
}

/* Calculate String View from '\0' terminated string */
SV sv_from_cstr(const char* cstr){
    return (SV) {
        .data = cstr,
        .count = strlen(cstr)
    };
}

/* Write a String View into the file descriptor */
ssize_t sv_write(SV sv, int fd){
    if(sv.data != NULL){
        return write(fd, sv.data, sv.count);
    }
    return -1;
}

SV sv_chop_while(SV* sv, bool (*pred)(char)){
    SV ret = {
        .data = sv->data
    };
    while (sv->count > 0 && pred(*sv->data)){
        sv->data++;
        sv->count--;
    }

    assert(sv->data >= ret.data);
    ret.count = (size_t) (sv->data - ret.data);
    
    return ret;
}

bool bool_notnewline(char c){
    return c != '\n';
}

SV sv_chop_line(SV* sv){
    SV ret = sv_chop_while(sv, bool_notnewline);

    if(*sv->data == '\n'){
        sv->data++;
        sv->count--;
    }

    return ret;
}

bool bool_notspace(char c){
    return !isspace(c);
}

SV sv_chop_word(SV* sv){
    return sv_chop_while(sv, bool_notspace);
}

void sv_trim_whitespace(SV* sv){
    while(sv->count > 0 && isspace(*sv->data)){
        sv->data++;
        sv->count--;
    }

    while(sv->count > 0 && isspace(sv->data[sv->count-1])){
        sv->count--;
    }
}

long sv_to_long(SV sv){
    char* endptr = NULL;
    long ret = strtol(sv.data, &endptr, 10);
    if(endptr != sv.data + sv.count){
        return 0;
    }
    return ret;
}

SV sv_to_upper(const char* cstr){
    char sp[strlen(cstr)];
    for(int i=0; strlen(cstr)>i; i++){
        /*
         * If current character is lowercase alphabet then
         * convert it to uppercase.
         */
        if(cstr[i]>='a' && cstr[i]<='z')
        {
            sp[i] = cstr[i] - 32;
        }
    }
    return sv_from_cstr(sp);
}

char* sv_dup(SV sv){
    char* ret = malloc(sv.count + 1);
    strncpy(ret, sv.data, sv.count);
    ret[sv.count] = '\0';
    return ret;
}