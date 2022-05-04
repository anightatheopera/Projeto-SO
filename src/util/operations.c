#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "operations.h"

static struct {
    char* name;
} operation_meta[] = {
    { .name = "bcompress"   },
    { .name = "bdecompress" },
    { .name = "gcompress"   },
    { .name = "gdecompress" },
    { .name = "encrypt"     },
    { .name = "decrypt"     },
    { .name = "nop"         } 
};

bool str_to_operation(const char* str, Operation* op){
    for(int i = 0; i < OPERATION_AMOUNT; i++){
        if(!strcmp(str, operation_meta[i].name)){
        *op = (Operation) i;
        return true;
        }
    }
    return false;
}

const char* operation_to_str(Operation op){
    assert(op < OPERATION_AMOUNT);
    return operation_meta[op].name;
}

#define OPERATIONS_SIZE 64

typedef struct operations {
    Operation vs[OPERATIONS_SIZE]; // valores
    size_t sz; // quantidade de valores no array
} Operations;

Operations* operations_new(){
    Operations* ret = malloc(sizeof(Operations));
    *ret = (Operations) { 0 };
    return ret;
}

bool operations_init(Operations* ops){
    *ops = (Operations) { 0 };
    return true;
}

bool operations_add(Operations* ops, Operation op){
    assert(ops->sz < OPERATIONS_SIZE && "Exceeded maximum buffer for operations.");
    ops->vs[ops->sz++] = op;
    return true;
}

Operation operations_get(Operations* ops, size_t i){
    assert(i < ops->sz && "Out of bounds access.");
    return ops->vs[i];
}

size_t operations_size(Operations* ops){
    return ops->sz;
}

void operations_free(Operations* ops){
    free(ops);
}

void op_mset_add(OperationMSet* mset1, OperationMSet* mset2){
    for (size_t i = 0; i < OPERATION_AMOUNT; i++){
        mset1->vs[i] += mset2->vs[i];
    }
}

OperationMSet operations_to_mset(Operations* ops){
    OperationMSet ret = { 0 };
    for (size_t i = 0; i < operations_size(ops); i++){
        ret.vs[operations_get(ops, i)]++;
    }
    return ret;
}

// current mset - request mset
// mset1 - mset2
void op_mset_sub(OperationMSet* mset1, OperationMSet* mset2){
    for (size_t i = 0; i < OPERATION_AMOUNT; i++){
        mset1->vs[i] -= mset2->vs[i];
    }
}

bool op_mset_lt(OperationMSet* mset1, OperationMSet* mset2, OperationMSet* max){
    for (size_t i = 0; i < OPERATION_AMOUNT; i++){
        if(mset1->vs[i] + mset2->vs[i] > max->vs[i]){
            return false;
        }
    }
    return true;
}