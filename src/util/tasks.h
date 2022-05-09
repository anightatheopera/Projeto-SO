#ifndef TASKS_H
#define TASKS_H

#include <stdlib.h>
#include <fcntl.h>

#include "communication.h"
#include "operations.h"

typedef struct {
    // o pedido que quer fazer
    Request req;
    // o numero de operaçoes que o pedido pede de cada operaçao
    OperationMSet mset;
    pid_t client;
    int client_fd[2];
    pid_t handler;
} Task;

typedef struct {
    Task* vs[64];
    size_t sz;
} Tasks;

Task* task_new(Request r, pid_t client, int client_fd[2]);

void tasks_add_running(Tasks* tasks, Task* newtask);
Task* tasks_remove_running(Tasks* tasks, pid_t handler);

void tasks_enqueue(Tasks* tasks, Task* newtask);
Task* tasks_remove(Tasks* tasks, size_t ind);

#endif