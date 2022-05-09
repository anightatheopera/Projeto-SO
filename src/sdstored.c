#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>

#include "util/sv.h"
#include "util/proc.h"
#include "util/utilities.h"
#include "util/communication.h"
#include "util/logger.h"
#include "util/tasks.h"

//SERVIDOR


/* Prints instructions on how to use the program */
void usage(int argc, char** argv){
    (void) argc;

    char buf[1024];
    snprintf(buf, sizeof(buf), "USAGE: %s [conf_file] [bin_path]\n", argv[0]);
    sv_write(sv_from_cstr(buf), STDOUT_FILENO);
}
/*
void parse_message(char* buf, int size){
    // ex:
    // buf is --> xxxxxx:proc-file;<priority>;file_in;file_out;filter_1;filter_2;...filter_n;

    char* pid = malloc(sizeof (char));
    char fifo[MAX_MESSAGE];
    int i,j;
    for ( i = 0; buf[i] != ':#ifndef PROC_H
#define PROC_H'; i++){
        pid = realloc(pid, sizeof(char)*(i+1));
        pid[i] = buf[i];
    }
    pid[i]='\0';
    // pid is --> xxxxxx
    RESPONSE_PIPE(fifo, pid);
    // fifo is --> /temp/pid
    if(mkfifo(fifo, 0666) == -1){
        perror(fifo);
    }
    i++;
    char message[size-i];
    for (j = 0; i<size; j++, i++){
        message[j] = buf[i];
    }
    // message is --> proc-file;<priority>;file_in;file_out;filter_1;filter_2;...filter_n;

}
*/

typedef struct {
    // path para os ficheiros binarios
    const char* bin_path;
    // define o numero maximo de instancias de cada executavel
    OperationMSet max_insts;
} ServerConfiguration;

// inicia no inicio da coisa
static ServerConfiguration server_configuration;

typedef struct {
    // quantidade de coisos que estao a correr
    OperationMSet curr_insts;
    // tasks que estao a correr
    Tasks running;
    // tasks que estao na fila
    Tasks in_queue;
} ServerState;

// inicializar a 0
static ServerState server_state = {
    .curr_insts = { .vs = { 0 } },
    .running = { .vs = { 0 } },
    .in_queue = { .vs = { 0 } },
};

char *strdup(const char *s);
Task* generate_debug_task(){
    Operations* ops =  operations_new();
    operations_add(ops, BCOMPRESS);
    operations_add(ops, BDECOMPRESS);
    operations_add(ops, NOP);
    operations_add(ops, NOP);

    Task* ret = malloc(sizeof(Task));

    ret->req = (Request) {
       .filepath_in = strdup("README.md"),
       .filepath_out = strdup("/tmp/test"),
       .priority = 0,
       .ops = ops
    };
    ret->mset = operations_to_mset(ops);

    return ret;
}

Task* next_runnable_task(){
    for(size_t i = 0; i < server_state.in_queue.sz; i++){
        if(op_mset_lte(&server_state.curr_insts, &server_state.in_queue.vs[i]->mset, &server_configuration.max_insts)){
            return tasks_remove(&server_state.in_queue, i);
        }
    }
    return NULL;
}

// inicia o handler 
void spawn_client_handler(Task* task){
    pid_t child_pid = fork();
    if(child_pid < 0){
        return;
    } else if(child_pid == 0){
        /* remove handler from parent */
        signal(SIGCHLD, SIG_DFL);
        Proc* procs = procs_run_operations(server_configuration.bin_path, task->req.filepath_in, task->req.filepath_out, task->req.ops);
        size_t procs_sz = operations_size(task->req.ops) + 2;
        for(size_t i = 0; i < procs_sz; i++){
            pid_t pid = waitpid(procs[i].pid, NULL, 0);
            logger_debug_fmt("awaited for %d.", pid);
        }
        sleep(2);
        exit(0);
    } else {
        logger_debug_fmt("running new task with operations " OPERATION_MSET_FMT, OPERATION_MSET_ARG(task->mset));

        op_mset_add(&server_state.curr_insts, &task->mset);
        tasks_add_running(&server_state.running, task);
        task->handler = child_pid;

        logger_debug_fmt("operations currently running " OPERATION_MSET_FMT, OPERATION_MSET_ARG(server_state.curr_insts));
    }
}

// da informaçoes sobre o coiso da folha parou 
void stopped_client_handler(int signum) {
    (void) signum;

    int pid = waitpid(-1, NULL, 0);
    logger_debug_fmt("child %d has stopped", pid);

    Task* task = tasks_remove_running(&server_state.running, pid);
    op_mset_sub(&server_state.curr_insts, &task->mset);

    signal(SIGCHLD, stopped_client_handler);

    while((task = next_runnable_task()) != NULL){
        spawn_client_handler(task);
    }
}


void parse_config(const char* config_filepath){
    // set max_insts unlimited by default
    for(size_t i = 0; i < OPERATION_AMOUNT; i++){
        server_configuration.max_insts.vs[i] = ULONG_MAX;
    }

    SV conf = sv_slurp_file(config_filepath);
    const char* conf_str = conf.data;

    while(conf.count > 0){
        SV line = sv_chop_line(&conf);
        char* opname = sv_dup(sv_chop_word(&line));
        Operation op;
        if(!str_to_operation(opname, &op)){
            logger_log_fmt("Invalid operation name '%s' in config.", opname);
            exit(1);
        }
        sv_trim_whitespace(&line);
        size_t max_inst = (size_t) sv_to_long(line);
        if(max_inst <= 0){
            max_inst = ULONG_MAX;
        }
        logger_log_fmt("Set maximum instances for operation '%s'(%d) to '%ld'", opname, op, max_inst);
        server_configuration.max_insts.vs[op] = max_inst;
        free(opname);
    }

    free((void*) conf_str);
}

/* Main */
int main(int argc, char** argv){
    (void) server_state;
    if(argc != 3){
        usage(argc, argv);
        exit(-1);
    }

    server_configuration.bin_path = argv[2];
    parse_config(argv[1]);
    logger_debug_fmt("max insts: " OPERATION_MSET_FMT, OPERATION_MSET_ARG(server_configuration.max_insts));

    signal(SIGCHLD, stopped_client_handler);
    /*
    if(mkfifo(CLIENT_SERVER_PIPE, 0666) == -1){
        perror(CLIENT_SERVER_PIPE);
    }
    */
    
    //spawn_client_handler(&task);
    char buf[100];
    ssize_t rd;
    while ((rd = read(STDIN_FILENO, buf, 100))){
        if(rd < 0){
           continue;
        }
        Task* task = generate_debug_task();
        tasks_enqueue(&server_state.in_queue, task);
        logger_debug_fmt("Amount of tasks in queue: %ld", server_state.in_queue.sz);
        while((task = next_runnable_task()) != NULL){
            spawn_client_handler(task);
        }
    }
   


    /*
    char buf[MAX_MESSAGE];
    int fd = open(CLIENT_SERVER_PIPE, O_RDONLY);
    while (1){
        size_t bytes_read = 0;
        while(!bytes_read ) 
           bytes_read = read(fd, buf, MAX_MESSAGE);
        parse_message(buf, bytes_read);
    }
    */


    //sv_write(conf, STDOUT_FILENO);
    return 0;
}
