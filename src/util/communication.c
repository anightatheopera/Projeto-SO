#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

#include "communication.h"

#define SERVER_DIR "/tmp/sdstored/"
#define SERVER_PATH SERVER_DIR "server"

/* Escreve uma String num File Descriptor */
bool str_write(const char* str, int fd){
    size_t len = strlen(str);
    return write(fd, &len, sizeof(size_t)) == sizeof(size_t) 
        && write(fd, str, len) == (ssize_t) len;
}

/* Lê uma String de um File Descriptor */
char* str_read(int fd){
    size_t len;
    if(read(fd, &len, sizeof(size_t)) != sizeof(size_t)){
        return NULL;
    }
    char* ret = malloc(len+1);
    if(read(fd, ret, len) != (ssize_t) len){
        free(ret);
        return NULL;
    }
    ret[len] = '\0';
    return ret;
}

/* Escreve um Request num File Descriptor */
bool request_write(Request* req, int fd){
    return str_write(req->filepath_in, fd)
        && str_write(req->filepath_out, fd)
        && operations_write(req->ops, fd)
        && write(fd, &req->priority, sizeof(req->priority)) == sizeof(req->priority);
}

/* Lê um Request de um File Descriptor */
bool request_read(Request* req, int fd){
    return (req->filepath_in = str_read(fd)) != NULL
        && (req->filepath_out = str_read(fd)) != NULL
        && (req->ops = operations_read(fd)) != NULL
        && read(fd, &req->priority, sizeof(req->priority)) == sizeof(req->priority);
}

/* Escreve um ClientMessage para um File Descriptor */
bool clientmsg_write(ClientMessage* cmsg, int fd){
    switch (cmsg->type){
        case REQUEST_STATUS:
            return write(fd, &cmsg->type, sizeof(cmsg->type));
        case REQUEST_OPERATIONS:
            return write(fd, &cmsg->type, sizeof(cmsg->type))
                && request_write(&cmsg->req, fd);
        default:
            assert(0 && "Unreachable!");
            return false;
    }
}

/* Lê um ClientMessage de um File Descriptor */
bool clientmsg_read(ClientMessage* cmsg, int fd){
    if(read(fd, &cmsg->type, sizeof(cmsg->type)) != sizeof(cmsg->type)){
        return false;
    }
    if(cmsg->type == REQUEST_STATUS){
        return true;
    }
    return request_read(&cmsg->req, fd);
}

/* Escreve um ServerMessage para um File Descriptor */
bool servermsg_write(ServerMessage* smsg, int fd){
    bool ret = true;
    ret = ret && write(fd, &smsg->type, sizeof(smsg->type)) == sizeof(smsg->type);
    if(smsg->type == RESPONSE_STATUS){
        return ret && write(fd, &smsg->status, sizeof(smsg->status)) == sizeof(smsg->status);
    }
    return ret;
}

/* Lê um ServerMessage de um File Descriptor */
bool servermsg_read(ServerMessage* smsg, int fd){
    bool ret = true;
    ret = ret && read(fd, &smsg->type, sizeof(smsg->type)) == sizeof(smsg->type);
    if(smsg->type == RESPONSE_STATUS){
        return ret && read(fd, &smsg->status, sizeof(smsg->status)) == sizeof(smsg->status);
    }
    return ret;
}

/* Altera as flags de um File Descriptor para o colocar como non-blocking */
int fd_set_nonblocking(int fd){
    int flags = fcntl(fd, F_GETFL);
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

/* Abre (e cria caso a flag 'create' seja true) um pipe com nome */
bool open_fifo(int pipefd[2], bool create, char* filename){
    if(create){
        mkfifo(filename, 0666);
    }
    if((pipefd[0] = open(filename, O_RDONLY | O_NONBLOCK)) < 0){
        return false;
    }
    if((pipefd[1] = open(filename, O_WRONLY)) < 0){
        close(pipefd[0]);
        return false;
    }
    fd_set_nonblocking(pipefd[0]);
    return true;
}

/* Abre (e cria caso a flag 'create' seja true) o pipe principal do servidor */
bool open_server(int pipefd[2], bool create){
    if(create){
        mkdir(SERVER_DIR, 0777);
    }
    return open_fifo(pipefd, create, SERVER_PATH);
}

/* Abre (e cria caso a flag 'create' seja true) o pipe que serve de comunicação de um cliente para o servidor */
bool open_client2server(pid_t client, int pipefd[2], bool create){
    static char buf[1024];
    snprintf(buf, 1024, SERVER_DIR "%d" "toserver", client);
    return open_fifo(pipefd, create, buf);
}

/* Abre (e cria caso a flag 'create' seja true) o pipe que serve de comunicação de um servidor para o cliente */
bool open_server2client(pid_t client, int pipefd[2], bool create){
    static char buf[1024];
    snprintf(buf, 1024, SERVER_DIR "toserver" "%d", client);
    return open_fifo(pipefd, create, buf);
}

/* Fecha as duas extremidades de um pipe */
void pipe_close(int pipefd[2]){
    close(pipefd[0]);
    close(pipefd[1]);
}