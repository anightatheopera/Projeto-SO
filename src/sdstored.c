#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "util/sv.h"
#include "util/proc.h"









//Macros e Ficheiros guardados aqui

#include "util/utilities.h"















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

    int fd = open("README.md", O_RDONLY);
    int fdout = open("/tmp/teste", O_WRONLY | O_CREAT, 0777);

    Proc reader;
    proc_reader(&reader, fd);
    close(fd);

    Proc cat;
    proc_exec_in(&cat, "/bin/cat", reader.out);

    Proc wc;
    proc_exec_in(&wc, "/bin/wc", cat.out);

    Proc writer;
    proc_writer(&writer, fdout, wc.out);
    close(fdout);
    
    int wstatus;
    proc_wait(&writer, &wstatus, 0);
    (void) wstatus;
    close(fdout);
    for(int i = 0; i < 3; i++){
        close(i);
    }
/*
    int pipeN[2];
    pipe(pipeN);
    Proc cat;
    proc_exec_in(&cat, "/bin/cat", pipeN[0]);

    char buf[5];
    
    write(pipeN[1], "cenas", 5);
    read(cat.out, buf, 5);
    close(cat.out);

    write(STDOUT_FILENO, buf, 5);

    close(pipeN[1]);
    int wstatus;
    proc_wait(&cat, &wstatus, 0);
    (void) wstatus;
*/
    //sv_write(conf, STDOUT_FILENO);
    return 0;
}
