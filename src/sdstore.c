#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "util/communication.h"
#include "util/logger.h"

/* Dá print quando há um timeout */
void timeout(int signum){
	(void) signum;
	logger_write("Response from server timedout.\n");
	exit(1);
}

/* Ultrapassa o primeiro argumento de argv */
char* shift(int* argc, char*** argv){
	if(*argc == 0){
		return NULL;
	}
	*argc -= 1;
	char* arg = **argv;
	*argv += 1;
	return arg;
}

/* Imprime a maneira de usar o programa */
void help(char* program){
	logger_write("Usage:\n");
	logger_write_fmt("\t%s status\n", program);
	logger_write_fmt("\t%s proc-file [PRIORITY]? [INPUT_FILE] [OUTPUT_FILE] [OPERATION]...\n", program);
	logger_write("Operations:\n");
	for(int i = 0; i < OPERATION_AMOUNT; i++){
		logger_write_fmt("\t%-20s%s\n", operation_to_str(i), operation_description(i));
	}
	exit(1);
}

/* Abre os pipes, mandando uma mensagem se tal não for possível */
void open_pipes(pid_t pid, int sv[2], int sv2c[2], int c2sv[2]){
	if(!open_server(sv, false) || !open_client2server(pid, c2sv, true) || !open_server2client(pid, sv2c, true)){
		logger_write("Failed to open the pipes to the server.\n");
		exit(1);
	}
}

/* Retira as informações sobre quais operações a executar e executa-as */
void send_operations_request(int c2sv[2], int argc, char** argv, char* program){
	Request req;
	req.priority = 0;
	req.filepath_in = shift(&argc, &argv);
	if(req.filepath_in[1] == '\0' && req.filepath_in[0] - '0' >= 0 && req.filepath_in[0] - '0' <= 5){
		req.priority = req.filepath_in[0] - '0';
		req.filepath_in = shift(&argc, &argv);
	}

	req.filepath_out = shift(&argc, &argv);

	if(argc <= 0){
		logger_write("Not enough arguments given.\n");
		help(program);
	}

	req.ops = operations_new();
	while(argc > 0){
		char* op_name = shift(&argc, &argv); 
		Operation op;
		if(!str_to_operation(op_name, &op)){
			logger_write_fmt("Invalid operation given '%s'.\n", op_name);
			help(program);
		}
    	operations_add(req.ops, op);
	}

    ClientMessage cmsg = { .type = REQUEST_OPERATIONS, .req = req };
    assert(clientmsg_write(&cmsg, c2sv[1]));
}

/* Manda um pedido para receber o status do servidor */
void send_status_request(int c2sv[2]){
	ClientMessage cmsg = { .type = REQUEST_STATUS };
    assert(clientmsg_write(&cmsg, c2sv[1]));
}

/* Imprime as informações de um request */
void print_requests(Request* reqs, size_t sz){
	size_t end = (sz < 5) ? sz : 5;
    for(size_t i = 0; i < end; i++){
        logger_write_fmt("- %d %s %s", reqs[i].priority, reqs[i].filepath_in, reqs[i].filepath_out);
		size_t ops_sz = operations_size(reqs[i].ops);
        for(size_t op_i = 0; op_i < ops_sz; op_i++){
			logger_write_fmt(" %s", operation_to_str(operations_get(reqs[i].ops, op_i)));
        }
        logger_write("\n");
    }
}

/* Imprime o status, dando informação sobre quais as mensagens a correr e quais estão pendentes */
void print_status(ServerMessageStatus* status){
    logger_write_fmt("Running Tasks: (%d)\n", status->running_tasks_sz);
	print_requests(status->running_tasks, status->running_tasks_sz);

	logger_write_fmt("\nPending Tasks: (%d)\n", status->pending_tasks_sz);
	print_requests(status->pending_tasks, status->pending_tasks_sz);

	logger_write("\nOperations:\n");
	for(int i = 0; i < OPERATION_AMOUNT; i++){
	    logger_write_fmt("- %s: %d/%d (running/max)\n", operation_to_str(i), status->running_ops.vs[i], status->maximum_ops.vs[i]);
	}
}

/* Espera pelas respostas do servidor e age conforme o tipo de resposta */
void handle_replies(int sv2c[2]){
	ServerMessage smsg;
	while(servermsg_read(&smsg, sv2c[0])){
		alarm(60);
		switch (smsg.type){
		case RESPONSE_STARTED:
			logger_write("processing\n");
			break;
		case RESPONSE_PENDING:
			logger_write("pending\n");
			break;
		case RESPONSE_STATUS:
			print_status(smsg.status);
			return;
		case RESPONSE_TERMINATED:
			logger_write("terminated\n");
			return;
		case RESPONSE_FINISHED:
			logger_write_fmt("concluded (bytes-input: %ld, bytes-output: %ld)\n", smsg.bytes_read, smsg.bytes_written);
			return;
		}
	}
}

int main(int argc, char** argv){
	signal(SIGALRM, timeout);
	char* program = shift(&argc, &argv);
	pid_t pid = getpid();
	if(argc == 0){
		help(program);
	}

	int sv[2], sv2c[2], c2sv[2];
	open_pipes(pid, sv, sv2c, c2sv);

	alarm(2);
	char* command = shift(&argc, &argv);
	if(!strcmp(command, "status")){
		send_status_request(c2sv);
	} else if(!strcmp(command, "proc-file")){
		send_operations_request(c2sv, argc, argv, program);
	} else {
		help(program);
	}

    write(sv[1], &pid, sizeof(pid_t));

	handle_replies(sv2c);
	
	return 0;
}
