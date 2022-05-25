#include <assert.h>

#include "tasks.h"

void task_free(Task* task){
    request_destroy(&task->req);
    pipe_close(task->cli2ser_pipe);
    pipe_close(task->ser2cli_pipe);
    free(task);
}

/* Remove uma Task de um certo índice de uma estrutura Tasks */
Task* tasks_remove(Tasks* tasks, size_t ind){
    Task* ret = tasks->vs[ind];
    tasks->sz--;
    for(size_t i = ind; i < tasks->sz; i++){
        tasks->vs[i] = tasks->vs[i+1];
    }
    return ret;
}

/* Adiciona uma Task à estrutura */
void tasks_add_running(Tasks* tasks, Task* task){
    assert(tasks->sz < sizeof(tasks->vs)/sizeof(tasks->vs[0]));
    tasks->vs[tasks->sz++] = task;
}

/* Remove uma Task que tem como handler o PID dado como argumento */
Task* tasks_remove_running(Tasks* tasks, pid_t handler){
    for(size_t i = 0; i < tasks->sz; i++){
        if(tasks->vs[i]->handler == handler){
            return tasks_remove(tasks, i);
        }
    }
    return NULL;
}

/* Adiciona uma Task à estrutura conforme a sua prioridade (Tasks com mais prioridade ficam num indice mais baixo) */
void tasks_enqueue(Tasks* tasks, Task* task){
    assert(tasks->sz < sizeof(tasks->vs)/sizeof(tasks->vs[0]));
    size_t i = tasks->sz;
    while(i > 0 && tasks->vs[i-1]->req.priority < task->req.priority){
        tasks->vs[i] = tasks->vs[i-1];
        i--;
    }
    tasks->vs[i] = task;
    tasks->sz++;
}
