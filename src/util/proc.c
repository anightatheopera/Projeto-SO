#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <assert.h>

#include "proc.h"

// TODO: ERROR HANDLING
int proc_exec(Proc* proc, const char* pathname){

    int pipe_in[2];
    assert(pipe(pipe_in) == 0);
    int pipe_out[2];
    assert(pipe(pipe_out) == 0);

    pid_t pid = fork();
    if(pid < 0){
        return (int) pid;
    } else if(pid == 0){
        close(pipe_in[1]);
        close(pipe_out[0]);

        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);

        int err = execl(pathname, pathname, NULL);
        /* Could not execute the command given in pathname */
        exit(err);
    } else {
        close(pipe_in[0]);
        close(pipe_out[1]);
        *proc = (Proc) {
            .pid = pid,
            .in = pipe_in[1],
            .out = pipe_out[0]
        };
        return 0;
    }
}

pid_t proc_wait(Proc* proc, int* wstatus, int options){
    return waitpid(proc->pid, wstatus, options);
}

void proc_close(Proc* proc){
    assert(close(proc->in) == 0);
    assert(close(proc->out) == 0);
}