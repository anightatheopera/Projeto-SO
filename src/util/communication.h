#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdlib.h>

#include "operations.h"

/* Especifica o tipo de pedido enviado do cliente para o servidor */
typedef enum {
    REQUEST_OPERATIONS,
    REQUEST_STATUS
} ClientMessageType;

/* Pedido de execução de operações do cliente */
typedef struct {
    char* filepath_in; // Caminho para o ficheiro que vai ser alterado
    char* filepath_out; // Caminho para o ficheiro alterado
    Operations* ops; // Operations aplicadas ao ficheiro de input
    int priority; // Prioridade do pedido
} Request;

/* Pedido enviado do cliente para o servidor */
typedef struct {
    ClientMessageType type; 
    Request req;
} ClientMessage;

/* Especifica o tipo de pedido enviado do servidor para o cliente */
typedef enum {
    RESPONSE_STATUS,
    RESPONSE_PENDING,
    RESPONSE_STARTED,
    RESPONSE_FINISHED,
    RESPONSE_TERMINATED
} ServerMessageType;

/* Informação devolvida ao cliente após um pedido de estado (REQUEST_STATUS) */
typedef struct {
    Request* running_tasks;
    size_t running_tasks_sz;
    Request* pending_tasks;
    size_t pending_tasks_sz;
    OperationMSet running_ops;
    OperationMSet maximum_ops;
} ServerMessageStatus;

/* Pedido enviado do servidor para o client */
typedef struct {
    ServerMessageType type;
    ServerMessageStatus* status;
} ServerMessage;

bool str_write(const char* str, int fd);
char* str_read(int fd);

bool request_write(Request* req, int fd);
bool request_read(Request* req, int fd);
void request_destroy(Request* req);

bool clientmsg_write(ClientMessage* cmsg, int fd);
bool clientmsg_read(ClientMessage* cmsg, int fd);

bool servermsg_write(ServerMessage* smsg, int fd);
bool servermsg_read(ServerMessage* smsg, int fd);

bool open_server(int pipefd[2], bool create);
bool open_client2server(pid_t client, int pipefd[2], bool create);
bool open_server2client(pid_t client, int pipefd[2], bool create);
int fd_set_nonblocking(int fd);
void pipe_close(int pipefd[2]);

#endif