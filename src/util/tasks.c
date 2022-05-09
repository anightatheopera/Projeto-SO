#include <assert.h>

#include "tasks.h"

Task* tasks_remove(Tasks* tasks, size_t ind){
    Task* ret = tasks->vs[ind];
    tasks->sz--;
    for(size_t i = ind; i < tasks->sz; i++){
        tasks->vs[i] = tasks->vs[i+1];
    }
    return ret;
}

void tasks_add_running(Tasks* tasks, Task* task){
    assert(tasks->sz < 63);
    tasks->vs[tasks->sz++] = task;
}


Task* tasks_remove_running(Tasks* tasks, pid_t handler){
    for(size_t i = 0; i < tasks->sz; i++){
        if(tasks->vs[i]->handler == handler){
            return tasks_remove(tasks, i);
        }
    }
    return NULL;
}

void tasks_enqueue(Tasks* tasks, Task* task){
    assert(tasks->sz < 63);
    size_t i = tasks->sz;
    while(i > 0 && tasks->vs[i-1]->req.priority < task->req.priority){
        tasks->vs[i] = tasks->vs[i-1];
        i--;
    }
    tasks->vs[i] = task;
    tasks->sz++;
}

