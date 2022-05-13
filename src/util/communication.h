#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdlib.h>

#include "operations.h"

typedef enum {
    REQUEST_OPERATIONS,
    REQUEST_STATUS
} ClientMessageType;

typedef struct {
    const char* filepath_in;
    const char* filepath_out;
    Operations* ops;
    int priority;
} Request;

typedef struct {
    ClientMessageType type;
    Request req;
} ClientMessage;

typedef enum {
    RESPONSE_STATUS,
    RESPONSE_PENDING,
    RESPONSE_STARTED,
    RESPONSE_FINISHED
} ServerMessageType;

typedef struct {
    size_t requests_being_processedq;
} ServerMessageStatus;

typedef struct {
    ServerMessageType type;
    ServerMessageStatus status;
} ServerMessage;

bool str_write(const char* str, int fd);
char* str_read(int fd);

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