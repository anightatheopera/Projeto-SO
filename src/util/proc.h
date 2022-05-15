#ifndef PROC_H
#define PROC_H

#include "operations.h"

typedef struct {
    pid_t pid; // ID do Processo
    int in; // FD que pode ser usado para escrever no STDIN do processo ou -1 se não for acessível
    int out; // FD que pode ser usado para ler do STDOUT do processo ou -1 se não for acessível
} Proc;

int proc_reader(Proc* proc, int fd);

int proc_writer(Proc* proc, int fd, int in);

int proc_exec_in(Proc* proc, const char* pathname, int in);

void proc_close(Proc* proc);

pid_t proc_wait(Proc* proc, int* wstatus, int options);

Proc* procs_run_operations(const char* filepath_prefix, const char* filepath_in, const char* filepath_out, Operations* ops);

#endif