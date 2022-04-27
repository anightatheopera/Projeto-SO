#ifndef PROC_H
#define PROC_H

#include "operations.h"

typedef struct {
    pid_t pid;
    int in; // coiso do stdin
    int out; //coiso do stdout
} Proc;

int proc_reader(Proc* proc, int fd);

int proc_writer(Proc* proc, int fd, int in);

/*
    proc - where to put the information of the created process
    pathname - the executable to run
    in - stdin for the process
*/
int proc_exec_in(Proc* proc, const char* pathname, int in);

void proc_close(Proc* proc);

pid_t proc_wait(Proc* proc, int* wstatus, int options);


/*
    filepath_prefix - pasta onde estao os programas
    filepath_in - ficheiro de input 
    filepath_out - ficheiro de output
    ops - operaçoes a executar ao ficheiro de input 
*/
Proc* procs_run_operations(const char* filepath_prefix, const char* filepath_in, const char* filepath_out, Operations* ops);

#endif