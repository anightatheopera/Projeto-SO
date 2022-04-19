#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <assert.h>

#include "proc.h"

int proc_reader(Proc* proc, int fd){
    int pipe_out[2];
    assert(pipe(pipe_out) == 0);

    pid_t pid = fork();
    if (pid < 0){
        return (int) pid;
    } else if (pid == 0){
        close(pipe_out[0]);
        char buf[1024];
        ssize_t rd;
        while((rd = read(fd, buf, sizeof(buf))) > 0){
            write(pipe_out[1], buf, (size_t) rd);
        }
        close(fd);
        close(pipe_out[1]);
        for(int i = 0; i < 3; i++){
            close(i);
        }
        exit(0);
    } else {
        close(pipe_out[1]);
        *proc = (Proc) {
            .pid = pid,
            .in = -1,
            .out = pipe_out[0]
        };
        return 0;
    }
}

int proc_writer(Proc* proc, int fd, int in){
    pid_t pid = fork();
    if (pid < 0){
        return (int) pid;
    } else if (pid == 0){
        char buf[1024];
        ssize_t rd;
        while((rd = read(in, buf, sizeof(buf))) > 0){
            write(fd, buf, (size_t) rd);
        }
        close(fd);
        close(in);
        for(int i = 0; i < 3; i++){
            close(i);
        }
        exit(0);
    } else {
        close(in);
        *proc = (Proc) {
            .pid = pid,
            .in = -1,
            .out = -1
        };
        return 0;
    }
}

//so ta a redirecionar os pipes
// TODO: ERROR HANDLING
int proc_exec_in(Proc* proc, const char* pathname, int in){
    int pipe_out[2];
    assert(pipe(pipe_out) == 0);

    pid_t pid = fork();
    if (pid < 0){
        return (int) pid;
    } else if (pid == 0){
        close(pipe_out[0]);

        dup2(in, STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);

        int err = execl(pathname, pathname, NULL);
        /* so da erro se a coisa em cima nao funfar */
        exit(err);
    } else {
        close(in);
        close(pipe_out[1]);
        *proc = (Proc) {
            .pid = pid,
            .in = -1,
            .out = pipe_out[0]
        };
        return 0;
    }
}



pid_t proc_wait(Proc* proc, int* wstatus, int options){
    return waitpid(proc->pid, wstatus, options);
}
