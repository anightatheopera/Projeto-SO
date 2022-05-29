#ifndef PROC_H
#define PROC_H

#include "operations.h"

typedef struct {
    pid_t pid; // ID do Processo
    int in; // FD que pode ser usado para escrever no STDIN do processo ou -1 se não for acessível
    int out; // FD que pode ser usado para ler do STDOUT do processo ou -1 se não for acessível
} Proc;

typedef struct {
    Proc* procs; // Processos executados (length = #Ops + 2)
    int write_reporter; // Pipe onde é escrita a quantidade de bytes escritos
    int read_reporter; // Pipe onde é escrita a quantidade de bytes lidos
} ProcsRunOps;

void proc_close(Proc* proc);

pid_t proc_wait(Proc* proc, int* wstatus, int options);

ProcsRunOps procs_run_operations(const char* filepath_prefix, const char* filepath_in, const char* filepath_out, Operations* ops);

#endif