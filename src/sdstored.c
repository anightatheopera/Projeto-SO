#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

#include "util/sv.h"
#include "util/proc.h"
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
    .running = { .vs = { 0 }, .sz = 0},
    .in_queue = { .vs = { 0 }, .sz = 0 },
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

bool can_run_task(Task* task){
    return op_mset_lte(&server_state.curr_insts, &task->mset, &server_configuration.max_insts);
}

Task* next_runnable_task(){
    for(size_t i = 0; i < server_state.in_queue.sz; i++){
        if(can_run_task(server_state.in_queue.vs[i])){
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
        {
            ServerMessage cmsg = { .type = RESPONSE_STARTED };
            servermsg_write(&cmsg, task->ser2cli_pipe[1]);
        }

        /* remove handler from parent */
        signal(SIGCHLD, SIG_DFL);
        Proc* procs = procs_run_operations(server_configuration.bin_path, task->req.filepath_in, task->req.filepath_out, task->req.ops);
        size_t procs_sz = operations_size(task->req.ops) + 2;
        for(size_t i = 0; i < procs_sz; i++){
            pid_t pid = waitpid(procs[i].pid, NULL, 0);
            logger_debug_fmt("awaited for %d.", pid);
        }

        {
            ServerMessage cmsg = { .type = RESPONSE_FINISHED };
            servermsg_write(&cmsg, task->ser2cli_pipe[1]);
        }
        //sleep(2);
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
    assert(task != NULL);
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

void test_client_request(){
    pid_t pid = getpid();
    Task* t = generate_debug_task();
    ClientMessage cmsg = { .type = REQUEST_OPERATIONS, .req = t->req };

    int ser2cli_pipe[2];
    open_server2client(pid, ser2cli_pipe, true);
    int cli2ser_pipe[2];
    open_client2server(pid, cli2ser_pipe, true);
    clientmsg_write(&cmsg, cli2ser_pipe[1]);

    int sv_pipe[2];
    open_server(sv_pipe, false);
    write(sv_pipe[1], &pid, sizeof(pid_t));
    
    ServerMessage smsg;
    while(servermsg_read(&smsg, ser2cli_pipe[0])){
        logger_debug_fmt("recv message %d", smsg.type);
        if(smsg.type == RESPONSE_FINISHED){
            exit(0);
        }
    }
    exit(-1);
}

Task* accept_client(pid_t client){
    Task* task = calloc(1, sizeof(Task));
    task->client = client;
    
    if(!open_client2server(client, task->cli2ser_pipe, false)){
        goto err_failed_open_c2s;
    }
    if(!open_server2client(client, task->ser2cli_pipe, false)){
        goto err_failed_open_s2c;
    }
    fd_set_nonblocking(task->cli2ser_pipe[0]);
    
    ClientMessage cmsg;
    if(!clientmsg_read(&cmsg, task->cli2ser_pipe[0])){
        goto err_failed_read;
    }
    
    switch (cmsg.type){
        case REQUEST_OPERATIONS:
            task->req = cmsg.req;
            task->mset = operations_to_mset(cmsg.req.ops);
            break;
        default:
            logger_log_fmt("Invalid message type %d", cmsg.type);
            goto err_invalid_msg;
    }

    return task;

err_invalid_msg:
err_failed_read:
err_failed_open_s2c:
    pipe_close(task->ser2cli_pipe);
err_failed_open_c2s:
    pipe_close(task->cli2ser_pipe);
    free(task);
    return NULL;
}

/* Main */
int main(int argc, char** argv){
    if(argc == 2 && !strcmp(argv[1], "--test-client")){
        test_client_request();
    }
    (void) server_state;
    if(argc != 3){
        usage(argc, argv);
        exit(-1);
    }

    server_configuration.bin_path = argv[2];
    parse_config(argv[1]);
    logger_debug_fmt("max insts: " OPERATION_MSET_FMT, OPERATION_MSET_ARG(server_configuration.max_insts));

    signal(SIGCHLD, stopped_client_handler);

    int sv_pipe[2];
    open_server(sv_pipe, true);
    while(1){

        pid_t client;
        ssize_t rd = read(sv_pipe[0], &client, sizeof(pid_t));

        if(rd < (ssize_t) sizeof(pid_t) || errno == EINTR){
            errno = 0;
            continue;
        }
        assert(rd >= 0);
        
        logger_log_fmt("Accepted client %d", client);
        Task* task = accept_client(client);

        if(can_run_task(task)){
            spawn_client_handler(task);
        } else {
            ServerMessage cmsg = { .type = RESPONSE_PENDING };
            servermsg_write(&cmsg, task->ser2cli_pipe[1]);
            tasks_enqueue(&server_state.in_queue, task);
        }
        
        logger_debug_fmt("%d %p", client, task);
    }

    return 0;
}
