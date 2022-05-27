#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "proc.h"
#include "logger.h"

/* Cria um processo que lê de um File Descriptor */ 
int proc_reader(Proc* proc, int fd, int report_read_fd){
    int pipe_out[2];
    assert(pipe(pipe_out) == 0);

    pid_t pid = fork();
    if (pid < 0){
        return (int) pid;
    } else if (pid == 0){
        close(pipe_out[0]);
        char buf[1024];
        ssize_t rd;
        size_t total = 0;
        while((rd = read(fd, buf, sizeof(buf))) > 0){
            write(pipe_out[1], buf, (size_t) rd);
            total += (size_t) rd;
        }
        write(report_read_fd, &total, sizeof(total));
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

/* Cria um processo que lê do File Descriptor in e escreve-o no File Descriptor fd */ 
int proc_writer(Proc* proc, int fd, int in, int report_written_fd){
    pid_t pid = fork();
    if (pid < 0){
        return (int) pid;
    } else if (pid == 0){
        char buf[1024];
        ssize_t rd;
        size_t total = 0;
        while((rd = read(in, buf, sizeof(buf))) > 0){
            write(fd, buf, (size_t) rd);
            total += (size_t) rd;
        }
        write(report_written_fd, &total, sizeof(total));
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

/* Cria um processo que executa um programa contido no pathname */ 
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
        logger_log_fmt("Could not execute '%s'.", pathname);
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

/* Fecha um File Descriptor */
void proc_close_out(Proc* proc){
    close(proc->out);
    proc->out = -1;
}

/* Espera que um dado processo termine */
pid_t proc_wait(Proc* proc, int* wstatus, int options){
    return waitpid(proc->pid, wstatus, options);
}

/* Executa as operações recebendo também os caminhos para os ficheiros de input e output e retorna os processos gerados */
ProcsRunOps procs_run_operations(const char* filepath_prefix, const char* filepath_in, const char* filepath_out, Operations* ops){
    int write_reporter[2];
    int read_reporter[2];
    
    int fd_in = open(filepath_in, O_RDONLY);
    if(fd_in < 0){
        goto err_open_file_in;
    }

    int fd_out = open(filepath_out, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if(fd_out < 0){
        goto err_open_file_out;
    }

    size_t ops_sz = operations_size(ops);
    Proc* ret = malloc(sizeof(Proc) * (ops_sz + 2));
    if(ret == NULL){
        goto err_failed_alloc;
    }

    assert(pipe(read_reporter) == 0);
    proc_reader(&ret[0], fd_in, read_reporter[1]);

    for(size_t i = 0; i < ops_sz; i++){
        static char filepath_buffer[4096]; // 4096 is the maximum size for a linux path https://serverfault.com/questions/9546/filename-length-limits-on-linux
        snprintf(filepath_buffer, 4096, "%s/%s", filepath_prefix, operation_to_str(operations_get(ops, i)));
        assert(proc_exec_in(&ret[i+1], filepath_buffer, ret[i].out) == 0);
        proc_close_out(&ret[i]);
    }

    assert(pipe(write_reporter) == 0);
    proc_writer(&ret[ops_sz+1], fd_out, ret[ops_sz].out, write_reporter[1]);
    proc_close_out(&ret[ops_sz]);

    close(fd_in);
    close(fd_out);

    return (ProcsRunOps) { .procs = ret, .read_reporter = read_reporter[0], .write_reporter = write_reporter[0] };

err_failed_alloc:
    close(fd_out);
err_open_file_out:
    close(fd_in);
err_open_file_in:
    return (ProcsRunOps) { .procs = NULL };
}
