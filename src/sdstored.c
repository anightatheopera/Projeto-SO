#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "util/sv.h"
#include "util/proc.h"


//SERVIDOR


char* get_trans_id(char* path){
    while(*path != '\0' && *path != ' '){
        path++;
    }
    while(*path == ' '){
        *(path++) = '\0';
    }
    return *path == '\0' ? NULL : path;
}

int get_max_inst(char* path){
    while(*path != '\0' && *path != ' '){
        path++;
    }
    if(*path++ != '\0'){
        return atoi(path);
    }
    else return 0;
}


/* Prints instructions on how to use the program */
void usage(int argc, char** argv){
    (void) argc;

    char buf[1024];
    snprintf(buf, sizeof(buf), "USAGE: %s [conf_file] [bin_path]\n", argv[0]);
    sv_write(sv_from_cstr(buf), STDOUT_FILENO);
}

/* Main */
int main(int argc, char** argv){
    if(argc != 3){
        usage(argc, argv);
        exit(-1);
    }
    SV conf = sv_slurp_file(argv[1]);
    (void) conf;

    /*
    Operations* ops =  operations_new();
    operations_add(ops, BCOMPRESS);
    operations_add(ops, BDECOMPRESS);
    operations_add(ops, NOP);
    operations_add(ops, NOP);

    procs_run_operations("bin","README.md","teste",ops);
    */

    while(conf.count > 0){
        SV line = sv_chop_line(&conf);
        sv_write(line, STDOUT_FILENO);
    }
    
    //sv_write(conf, STDOUT_FILENO);
    return 0;
}
