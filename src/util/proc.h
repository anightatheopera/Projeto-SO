#ifndef PROC_H
#define PROC_H

#include <sys/wait.h>

typedef struct {
    pid_t pid;
    int in; // coiso do stdin
    int out; //bcoiso do stdout
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

#endif