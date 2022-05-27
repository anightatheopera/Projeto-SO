#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include "util/sv.h"
#include "util/proc.h"
#include "util/communication.h"
#include "util/logger.h"
#include "util/tasks.h"

/* Imprime as indicações de como usar o programa */
void usage(int argc, char** argv){
    (void) argc;

    char buf[1024];
    snprintf(buf, sizeof(buf), "USAGE: %s [conf_file] [bin_path]\n", argv[0]);
    sv_write(sv_from_cstr(buf), STDOUT_FILENO);
}

/* Configuração do servidor */
typedef struct { 
    const char* bin_path; // Caminho para os ficheiros binários 
    OperationMSet max_insts; // Define o número máximo de instâncias de cada executável
} ServerConfiguration;


static ServerConfiguration server_configuration; // Configuração do servidor

/* Estados de spawn de um gestor de cliente */
typedef enum {
    SPAWN_STATE_NONE, // o servidor nao esta a spawnar um gestor atualmente
    SPAWN_STATE_SPAWNING, // o servidor esta a spawnar um gestor
    SPAWN_STATE_SPAWNING_SIGNAL, // o servidor recebeu um signal enquanto spawnava um gestor
} SpawnState;

/* Estado do servidor */
typedef struct {
    OperationMSet curr_insts; // Quantidade de operações a correr
    Tasks running; // Tasks que estão a correr
    Tasks in_queue; // Tasks que estão à espera de correr
    bool terminated; // Indica se o servidor recebeu SIGTERM
    int sv_pipe[2]; // FIFO onde o servidor recebe pedidos de todos os clientes
    SpawnState spawn_state; // indica se o servidor esta atualmenta a spawnar um handler
} ServerState;

/* Inicializa o estado do servidor a 0 */
static ServerState server_state = {
    .curr_insts = { .vs = { 0 } },
    .running = { .vs = { 0 }, .sz = 0},
    .in_queue = { .vs = { 0 }, .sz = 0 },
    .terminated = false,
    .spawn_state = SPAWN_STATE_NONE
};

/* Sabendo o número máximo de instâncias dos executáveis, retorna True se ainda puder a Task às Tasks a correr */
bool can_run_task(Task* task){
    return op_mset_lte(&server_state.curr_insts, &task->mset, &server_configuration.max_insts);
}

/* Retorna a próxima Task a poder ser corrida */
Task* next_runnable_task(){
    for(size_t i = 0; i < server_state.in_queue.sz; i++){
        if(can_run_task(server_state.in_queue.vs[i])){
            return tasks_remove(&server_state.in_queue, i);
        }
    }
    return NULL;
}

/* Inicia o gestor do cliente */  
void spawn_client_handler(Task* task){
    server_state.spawn_state = SPAWN_STATE_SPAWNING;

    pid_t child_pid = fork();
    if(child_pid < 0){
        return;
    } else if(child_pid == 0){
        {
            ServerMessage cmsg = { .type = RESPONSE_STARTED };
            servermsg_write(&cmsg, task->ser2cli_pipe[1]);
        }

        // Desativa os sinais no gestor
        signal(SIGCHLD, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        ProcsRunOps procs_run_result =
            procs_run_operations(server_configuration.bin_path, task->req.filepath_in, task->req.filepath_out, task->req.ops);
        Proc* procs = procs_run_result.procs;
        size_t procs_sz = operations_size(task->req.ops) + 2;
        for(size_t i = 0; i < procs_sz; i++){
            pid_t pid = waitpid(procs[i].pid, NULL, 0);
            (void) pid;
        }
#ifdef DEBUG
        logger_debug("Compiled with DEBUG flag defined, sleeping for 1 second.");
        sleep(2);
#endif
        size_t bytes_read = 0;
        size_t bytes_written = 0;
        read(procs_run_result.read_reporter, &bytes_read, sizeof(size_t));
        read(procs_run_result.write_reporter, &bytes_written, sizeof(size_t));
        logger_log_fmt("READ %ld from '%s'; WRITTEN %ld to '%s'", bytes_read, task->req.filepath_in, bytes_written, task->req.filepath_out);
        {
            ServerMessage cmsg = { .type = RESPONSE_FINISHED, .bytes_read = bytes_read, .bytes_written = bytes_written };
            servermsg_write(&cmsg, task->ser2cli_pipe[1]);
        }
        exit(0);
    } else {
        logger_debug_fmt("running new task with operations " OPERATION_MSET_FMT " and priority %d", OPERATION_MSET_ARG(task->mset), task->req.priority);
    
        tasks_add_running(&server_state.running, task);
        task->handler = child_pid;
        op_mset_add(&server_state.curr_insts, &task->mset);
    
    }
    if(server_state.spawn_state == SPAWN_STATE_SPAWNING_SIGNAL){
        server_state.spawn_state = SPAWN_STATE_NONE;
        raise(SIGUSR1);
    }
    server_state.spawn_state = SPAWN_STATE_NONE;
}

/* Desliga o servidor se não tiver tarefas a executar */
void attempt_shutdown(){
    if(server_state.running.sz == 0){
        pipe_close(server_state.sv_pipe);
        exit(0);
    }
}

/* Trata do sinal de término */
void sigterm_handler(int signum){
    (void) signum;
    server_state.terminated = true;
    logger_log("Server has recieved shutdown signal.");
    attempt_shutdown();
}

/* Remove um gestor de cliente cuja execução terminou */
void remove_handler(pid_t handler){
    Task* task = tasks_remove_running(&server_state.running, handler);
    if(task == NULL){
        return;
    }

    op_mset_sub(&server_state.curr_insts, &task->mset);
    task_free(task);
}

/* Verifica se há gestores terminados e, se tal acontecer, remove-os */
void check_dead_handlers(int signum) {
    if(server_state.spawn_state != SPAWN_STATE_NONE){
        server_state.spawn_state = SPAWN_STATE_SPAWNING_SIGNAL;
        return;
    }
    signal(SIGCHLD, check_dead_handlers);
    signal(SIGALRM, check_dead_handlers);
    signal(SIGUSR1, check_dead_handlers);
    alarm(5);
    (void) signum;

    pid_t handler;
    while((handler = waitpid(-1, NULL, WNOHANG)) > 0){
        //logger_debug_fmt("child %d has stopped", pid);
        remove_handler(handler);
    }

    Task* task;
    while((task = next_runnable_task()) != NULL){
        spawn_client_handler(task);
    }

    if(server_state.terminated){
        attempt_shutdown();
    }
    logger_log_fmt("operations currently running " OPERATION_MSET_FMT, OPERATION_MSET_ARG(server_state.curr_insts));
}

/* Interpreta a configuração */
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

/* Envia a resposta com o status atual do servidor */
void send_status_response(int fd){
    ServerMessage smsg = (ServerMessage) { .type = RESPONSE_STATUS };
    smsg.status = malloc(sizeof(ServerMessageStatus));

    smsg.status->running_tasks = malloc(sizeof(Request) * server_state.running.sz);
    smsg.status->running_tasks_sz = server_state.running.sz;
    for(size_t i = 0; i < server_state.running.sz; i++){
        smsg.status->running_tasks[i] = server_state.running.vs[i]->req;
    }

    smsg.status->pending_tasks = malloc(sizeof(Request) * server_state.in_queue.sz);
    smsg.status->pending_tasks_sz = server_state.in_queue.sz;
    for(size_t i = 0; i < server_state.in_queue.sz; i++){
        smsg.status->pending_tasks[i] = server_state.in_queue.vs[i]->req;
    }

    smsg.status->running_ops = server_state.curr_insts;
    smsg.status->maximum_ops = server_configuration.max_insts;

    servermsg_write(&smsg, fd);

    free(smsg.status->pending_tasks);
    free(smsg.status->running_tasks);
    free(smsg.status);
}

/* Aceita o cliente */
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
    fd_set_nonblocking(task->ser2cli_pipe[1]);
    
    if(server_state.terminated){
        ServerMessage smsg = (ServerMessage) { .type = RESPONSE_TERMINATED };
        servermsg_write(&smsg, task->ser2cli_pipe[1]);
        goto server_terminated;
    }

    ClientMessage cmsg;
    if(!clientmsg_read(&cmsg, task->cli2ser_pipe[0])){
        goto err_failed_read;
    }
    
    switch (cmsg.type){
        case REQUEST_OPERATIONS:
            task->req = cmsg.req;
            task->mset = operations_to_mset(cmsg.req.ops);
            break;
        case REQUEST_STATUS:
            send_status_response(task->ser2cli_pipe[1]);
            goto status_requested;
        default:
            logger_log_fmt("Invalid message type %d", cmsg.type);
            goto err_invalid_msg;
    }

    return task;
    
status_requested:
err_invalid_msg:
err_failed_read:
server_terminated:
err_failed_open_s2c:
    pipe_close(task->ser2cli_pipe);
err_failed_open_c2s:
    pipe_close(task->cli2ser_pipe);
    free(task);
    return NULL;
}


int main(int argc, char** argv){
    (void) server_state;
    if(argc != 3){
        usage(argc, argv);
        exit(-1);
    }

    server_configuration.bin_path = argv[2];
    parse_config(argv[1]);
    logger_debug_fmt("max insts: " OPERATION_MSET_FMT, OPERATION_MSET_ARG(server_configuration.max_insts));

    signal(SIGCHLD, check_dead_handlers);
    signal(SIGTERM, sigterm_handler);
    signal(SIGUSR1, sigterm_handler);

    assert(open_server(server_state.sv_pipe, true));
    while(1){

        pid_t client;
        ssize_t rd = read(server_state.sv_pipe[0], &client, sizeof(pid_t));

        if(rd < (ssize_t) sizeof(pid_t) || errno == EINTR){
            errno = 0;
            continue;
        }
        assert(rd >= 0);
        
        logger_log_fmt("Received client %d", client);
        Task* task = accept_client(client);
        if(task == NULL){
            continue;
        }

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
