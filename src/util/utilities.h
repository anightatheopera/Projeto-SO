#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_MESSAGE 1024

#define RESPONSE_PIPE(str,pid) snprintf(str, 20, "/tmp/%s", pid)
#define CLIENT_SERVER_PIPE "/tmp/client_server_pipe"
#define WRITE_LITERAL(fd, str) write(fd, str, sizeof(str))
#define COMPARE_STRING(str1, str2) !strcmp(str1,str2)
