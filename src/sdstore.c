#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string.h>

#include "util/sv.h"
#include "util/proc.h"
#include "util/communication.h"
#include "util/logger.h"
#include "util/tasks.h"
#include "util/logger.h"

#define WRITE_LITERAL_DEBUG(fd, str) WRITE_LITERAL(fd, str)

#define WRITE_LITERAL(fd, str) write(fd, str, sizeof(str))
#define COMPARE_STRING(str1, str2) !strcmp(str1,str2)


void print_help(){
	WRITE_LITERAL(1, "\nUsage : ./sdstore <proc-file | status>\n\n");
	WRITE_LITERAL(1, "\tProcess File \n\t\t./sdstore proc-file <priority> <input | output> < ALGORITHMS>\n\n");
	WRITE_LITERAL(1, "\tALGORITHMS\n\t\t - bcompress | bdecompress   -> Compress or Decompress file in BZIP format\n\t\t - gcompress | gdecompress   -> Compress or Decompress file in GZIP format\n\t\t - encrypt   | decrypt       -> Encrypt or Decrypt file\n\t\t - nop                       -> Copy the data without change (copy)\n\n");
	WRITE_LITERAL(1, "\tStatus \n\t\t./sdstore status\n");
}

void alarmhandler(int signum) {
	WRITE_LITERAL(1, "Timeout trying to reach the server.\n");
	exit(signum);
}

int main (int argc,char** argv) {
	signal(SIGALRM, alarmhandler); // error handling connection timeout
	
	if (argc >= 2 && (COMPARE_STRING(argv[1], "status") || COMPARE_STRING(argv[1],"proc-file"))){

		// abrir main pipe;
		//enviar o PID
		// abrir pipe pidtoserver
		// enviar mensagem com o formato ClientMessage
		// abrir pipe resposta
		// ler resposta com o formato ServerMessage
		// transformar o formato em algo readable

		//alarms caso não consiga conectar


		ClientMessage* in = calloc(1, sizeof(ClientMessage));
		

		if(COMPARE_STRING(argv[1], "status")){
			in->type = REQUEST_STATUS;
		}
		else if (COMPARE_STRING(argv[1],"proc-file")){

			in->type = REQUEST_OPERATIONS;

			Operations* ops = operations_new();

			// read algorithms
			for (int i = 5; i < argc; i++) {
				Operation* o = calloc(1,sizeof(Operation));
				operations_add(ops,str_to_operation(argv[i],o));
			}
			SV in_s = sv_from_cstr(argv[3]);
			SV out_s = sv_from_cstr(argv[4]);

			in->req = (Request) {
				.filepath_in = sv_dup(in_s),
       			.filepath_out = sv_dup(out_s),
       			.priority = atoi(argv[2]),
       			.ops = ops
			};
		}
		else goto errors;
		
		//get client pid
		pid_t main_pid = getpid(); 
		logger_write_fmt("%d\n", main_pid);

		int status = 0;
		WRITE_LITERAL(1, "Connecting to the server...\n");
		int main_fifo[2];
		int opened = open_server(main_fifo,false);
		if (!fork()) {
			if (!opened) {
				perror("Server-Main-Fifo");
				_exit(5);
			}
			write(main_fifo[1],&main_pid,sizeof(pid_t));
			close(main_fifo[1]);
			close(main_fifo[0]);
			_exit(1);
		}
		
		alarm(5); //Mata cliente caso a conexão demore mais de 5 segundos;

		wait(&status);
		while(WEXITSTATUS(status)!=1);
		alarm(0); // Reset ao alarm para não matar o cliente caso este obtenha resposta;
		close(main_fifo[1]);
		close(main_fifo[0]);

		int client_2_server[2];
		bool clientserver = false;
		alarm(10); //Mata cliente caso a conexão demore mais de 10 segundos;
		while(!clientserver){
			clientserver = open_client2server(main_pid,client_2_server,false);
			}
		alarm(0); // Reset ao alarm para não matar o cliente caso este obtenha resposta;
		if (!fork()) {
			if (!clientserver) {
				perror("Client_2_Server");
				_exit(5);
			}
			clientmsg_write(in,client_2_server[1]);
			close(client_2_server[1]);
			close(client_2_server[0]);
			_exit(1);
		}
		alarm(5); //Mata cliente caso a conexão demore mais de 5 segundos;

		wait(&status);
		while(WEXITSTATUS(status)!=1);
		alarm(0); // Reset ao alarm para não matar o cliente caso este obtenha resposta;
		close(client_2_server[1]);
		close(client_2_server[0]);
		
		WRITE_LITERAL(1, "Message sent to the server.\n");

		// Resposta do servidor;
		int server_response_fds[2];
		bool response = false;

		alarm(15); //Mata cliente caso a resposta demore mais de 15 segundos;
		while(!response)
			 response = open_server2client(main_pid,server_response_fds,false);
		alarm(0); // Reset ao alarm para não matar o cliente caso este obtenha resposta;

		ServerMessage* out = calloc(1, sizeof(ServerMessage));
		bool suc = servermsg_read(out,server_response_fds[0]);
		if (suc){
			servermsg_write(out,STDOUT_FILENO);
		}
		close(server_response_fds[0]);
		close(server_response_fds[1]);
	}
	else
errors:
		print_help(); // Print clientside usage
	
	return 0;
}
