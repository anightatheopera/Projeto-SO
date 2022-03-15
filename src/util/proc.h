#ifndef PROC_H
#define PROC_H

#include <sys/wait.h>

typedef struct {
    pid_t pid;
    int in;
    int out;
} Proc;

int proc_exec(Proc* proc, const char* pathname);

void proc_close(Proc* proc);

pid_t proc_wait(Proc* proc, int* wstatus, int options);

#endif