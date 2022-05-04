#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "proc.h"
#include "logger.h"

/* função para ler de um processo */ 
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

/* função para escrever para um processo */ 
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

/* executa um processo */ 
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
        // so da erro se a coisa em cima nao funfar 
        exit(err);
    } else {
        logger_debug_fmt("executing %s", pathname);
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

void proc_close_out(Proc* proc){
    close(proc->out);
    proc->out = -1;
}

/* espera que o processo acabe */
pid_t proc_wait(Proc* proc, int* wstatus, int options){
    return waitpid(proc->pid, wstatus, options);
}

/* executa as operaçoes e retorna os processos gerados */
Proc* procs_run_operations(const char* filepath_prefix, const char* filepath_in, const char* filepath_out, Operations* ops){
    
    int fd_in = open(filepath_in, O_RDONLY);
    if(fd_in < 0){
        goto err_open_file_in;
    }

    int fd_out = open(filepath_out, O_WRONLY | O_CREAT, 0664);
    if(fd_out < 0){
        goto err_open_file_out;
    }

    size_t ops_sz = operations_size(ops);
    Proc* ret = malloc(sizeof(Proc) * (ops_sz + 2));
    if(ret == NULL){
        goto err_failed_alloc;
    }

    proc_reader(&ret[0], fd_in);

    for(size_t i = 0; i < ops_sz; i++){
        static char filepath_buffer[4096]; // 4096 is the maximum size for a linux path https://serverfault.com/questions/9546/filename-length-limits-on-linux
        snprintf(filepath_buffer, 4096, "%s/%s", filepath_prefix, operation_to_str(operations_get(ops, i)));
        proc_exec_in(&ret[i+1], filepath_buffer, ret[i].out);
        proc_close_out(&ret[i]);
    }

    proc_writer(&ret[ops_sz+1], fd_out, ret[ops_sz].out);
    proc_close_out(&ret[ops_sz]);

    close(fd_in);
    close(fd_out);

    return ret;

err_failed_alloc:
    close(fd_out);
err_open_file_out:
    close(fd_in);
err_open_file_in:
    return NULL;
}
