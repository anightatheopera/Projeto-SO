#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <string.h>

#define WRITE_LITERAL(fd, str) write(fd, str, sizeof(str))

//CLIENTE

void print_help(){
	WRITE_LITERAL(1, "\nUsage : ./sdstore <proc-file | status>\n\n");
	WRITE_LITERAL(1, "\tProcess File \n\t\t./sdstore proc-file <priority> <input | output> < ALGORITHMS>\n\n");
	WRITE_LITERAL(1, "\tALGORITHMS\n\t\t - bcompress | bdecompress   -> Compress or Decompress file in BZIP format\n\t\t - gcompress | gdecompress   -> Compress or Decompress file in GZIP format\n\t\t - encrypt   | decrypt       -> Encrypt or Decrypt file\n\t\t - nop                       -> Copy the data without change (copy)\n\n");
	WRITE_LITERAL(1, "\tStatus \n\t\t./sdstore status\n");
}

void sighandler(int signum) {
	WRITE_LITERAL(1, "Timeout trying to reach the server.\n");
	exit(1);
}

int main (int argc,char** argv) {
	signal(SIGALRM, sighandler); // error handling connection timeout
	switch (argc){
		case 2:
			if (!strcmp(argv[1],"status")){ // need to compare if the argv[1] is "status" else send it to default (i will use GOTO its not that bad)

			}
			else goto DEFAULT_JUMP;
			break;
DEFAULT_JUMP:
		default:
			print_help(); // Print clientside usage
			break;
	}

	return 0;
}
