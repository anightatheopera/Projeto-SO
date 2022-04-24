#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <limits.h>

#include "util/utilities.h"

//CLIENTE

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
		char message[MAX_MESSAGE];
		char pid_str[15];
		snprintf(pid_str, 15, "%d", getpid());
		snprintf(message, 32, "%s:", pid_str);

		for (int i = 1; i < argc; i++) {
			strcat(message, argv[i]);
			strcat(message, ";");
		}

		int status = 0;
		WRITE_LITERAL(1, "Connecting to the server...\n");
		
		if (!fork()) {
			int server_fd = open(CLIENT_SERVER_PIPE, O_WRONLY);
			if (server_fd == -1) {
				perror(CLIENT_SERVER_PIPE);
				_exit(5);
			}
			write(server_fd, message, strlen(message));
			close(server_fd);
			_exit(1);
		}
		
		alarm(5); //Mata cliente caso a conexão demore mais de 5 segundos;
		wait(&status);
		while(WEXITSTATUS(status)!=1);
		alarm(0); // Reset ao alarm para não matar o cliente caso este obtenha resposta;
		
		WRITE_LITERAL(1, "Message sent to the server.\n");

		// Resposta do servidor;
		int server_response_fd = -1;
		char server_response_pipe[32];
		RESPONSE_PIPE(server_response_pipe,pid_str);

		alarm(10); //Mata cliente caso a resposta demore mais de 10 segundos;
		while(server_response_fd == -1)
			server_response_fd = open(server_response_pipe, O_RDONLY);
		alarm(0); // Reset ao alarm para não matar o cliente caso este obtenha resposta;
		
		char buf[10024];
		ssize_t bytes_read = read(server_response_fd, buf, 10024);
		snprintf(buf,32,"%ld bytes received.\n", bytes_read); 
		WRITE_LITERAL(1, buf);
		while(bytes_read) {
			write(1, buf, bytes_read);
			bytes_read = read(server_response_fd, buf, 10024);
		}

		close(server_response_fd);
		unlink(server_response_pipe);
	}
	else
		print_help(); // Print clientside usage
	
	return 0;
}
