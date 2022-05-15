#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

#include "sv.h"

/* Retorna uma String View vazia */
SV sv_empty(){
    return (SV) {
        .data = NULL,
        .count = 0
    };
}

/* Carrega todos os conteúdos de um ficheiro para uma String View */
SV sv_slurp_file(const char* filename){
    /* Open the file */
    int fd = open(filename, O_RDONLY);
    if(fd < 0){
        goto err_open;
    }

    /* Obtém o tamanho do ficheiro */
    struct stat s;
    int status = fstat(fd, &s);
    size_t size = (size_t) s.st_size;
    if(status < 0){
        goto err_stat;
    }
    
    /* Aloca a memória necessária para a String */
    char* data = (char*) malloc(size * sizeof(char));
    if(data == NULL){
        goto err_malloc;
    }

    /* Lê o ficheiro para a String */
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

/* Calcula uma String View através de uma String terminada em '\0' */
SV sv_from_cstr(const char* cstr){
    return (SV) {
        .data = cstr,
        .count = strlen(cstr)
    };
}

/* Escreve uma String View para um File Descriptor */
ssize_t sv_write(SV sv, int fd){
    return write(fd, sv.data, sv.count);
}

/* Corta carateres enquanto o predicado é verdadeiro, retornando uma String View com os carateres cortados */
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

/* Verifica se um carater não é igual a '\n' */
bool bool_notnewline(char c){
    return c != '\n';
}

/* Corta uma linha do input, alterando o mesmo */
SV sv_chop_line(SV* sv){
    SV ret = sv_chop_while(sv, bool_notnewline);

    if(*sv->data == '\n'){
        sv->data++;
        sv->count--;
    }
    return ret;
}

/* Verfica se um carater não é um espaço */
bool bool_notspace(char c){
    return !isspace(c);
}

/* Corta uma palavra do input, alterando o mesmo */
SV sv_chop_word(SV* sv){
    return sv_chop_while(sv, bool_notspace);
}

/* Retira os espaços à esquerda e à direita de uma String View */
void sv_trim_whitespace(SV* sv){
    while(sv->count > 0 && isspace(*sv->data)){
        sv->data++;
        sv->count--;
    }

    while(sv->count > 0 && isspace(sv->data[sv->count-1])){
        sv->count--;
    }
}

/* Converte uma String View para um Long */
long sv_to_long(SV sv){
    char* endptr = NULL;
    long ret = strtol(sv.data, &endptr, 10);
    if(endptr != sv.data + sv.count){
        return 0;
    }
    return ret;
}

/* Duplica o String View, retornando o resultado numa String */
char* sv_dup(SV sv){
    char* ret = malloc(sv.count + 1);
    strncpy(ret, sv.data, sv.count);
    ret[sv.count] = '\0';
    return ret;
}