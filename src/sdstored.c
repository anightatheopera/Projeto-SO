#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    Proc cat;
    proc_exec(&cat, "/bin/cat");

    char buf[5];
    
    write(cat.in, "cenas", 5);
    read(cat.out, buf, 5);

    write(STDOUT_FILENO, buf, 5);
    
    proc_close(&cat);

    int wstatus;
    proc_wait(&cat, &wstatus, 0);
    (void) wstatus;
*/

    //sv_write(conf, STDOUT_FILENO);
    return 0;
}