#ifndef TASKS_H
#define TASKS_H

#include <stdlib.h>
#include <fcntl.h>

#include "communication.h"
#include "operations.h"

/* Tarefa a ser executada a pedido de um cliente */
typedef struct {
    Request req; // O pedido
    OperationMSet mset; // O multi set indicativo da quantidade de operações pedidas
    pid_t client; // PID do cliente 
    int cli2ser_pipe[2]; // Pipe utilizado para o cliente comunicar com o servidor
    int ser2cli_pipe[2]; // Pipe utilizado para o servidor comunicar com o cliente
    pid_t handler; // PID do processo que está a executar a tarefa
} Task;

/* Estrutura de tarefas */
typedef struct {
    Task* vs[1000]; // Array de Task
    size_t sz; // Quantidade de Task no Array
} Tasks;

void task_free(Task* task);

void tasks_add_running(Tasks* tasks, Task* newtask);
Task* tasks_remove_running(Tasks* tasks, pid_t handler);

void tasks_enqueue(Tasks* tasks, Task* newtask);
Task* tasks_remove(Tasks* tasks, size_t ind);

#endif