#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "operations.h"

/* Meta dados relativos a cada operação */
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

/* Transforma uma String numa Operation */
bool str_to_operation(const char* str, Operation* op){
    for(int i = 0; i < OPERATION_AMOUNT; i++){
        if(!strcmp(str, operation_meta[i].name)){
        *op = (Operation) i;
        return true;
        }
    }
    return false;
}

/* Transforma uma Operation numa String */
const char* operation_to_str(Operation op){
    assert(op < OPERATION_AMOUNT);
    return operation_meta[op].name;
}

/* Número máximo de Operation */
#define OPERATIONS_SIZE 64

/* Array de Operation */
typedef struct operations {
    Operation vs[OPERATIONS_SIZE]; // Array dos índices das Operation
    size_t sz; // Quantidade de valores no array
} Operations;

/* Cria uma nova estrutura Operations */
Operations* operations_new(){
    Operations* ret = malloc(sizeof(Operations));
    *ret = (Operations) { 0 };
    return ret;
}

/* Inicia uma estrutura Operations */
bool operations_init(Operations* ops){
    *ops = (Operations) { 0 };
    return true;
}

/* Adiciona uma Operation a uma estrutura Operations */
bool operations_add(Operations* ops, Operation op){
    assert(ops->sz < OPERATIONS_SIZE && "Exceeded maximum buffer for operations.");
    ops->vs[ops->sz++] = op;
    return true;
}

/* Retorna a Operation de índice i da estrutura Operations */
Operation operations_get(Operations* ops, size_t i){
    assert(i < ops->sz && "Out of bounds access.");
    return ops->vs[i];
}

/* Retorna a quantidade de Operation na dada estrutura Operations */
size_t operations_size(Operations* ops){
    return ops->sz;
}

/* Liberta a memória de uma estrutura Operations */
void operations_free(Operations* ops){
    free(ops);
}

/* Escreve uma estrutura Operations num File Descriptor */
bool operations_write(const Operations* ops, int fd){
    return write(fd, ops, sizeof(Operations)) == sizeof(Operations);
}

/* Lê uma estrutura Operations de um File Descriptor */
Operations* operations_read(int fd){
    Operations* ret = malloc(sizeof(Operations));
    if(read(fd, ret, sizeof(Operations)) != sizeof(Operations)){
        free(ret);
        return NULL;
    }
    return ret;
}

/* Transforma uma estrutura Operations num multi set */
OperationMSet operations_to_mset(Operations* ops){
    OperationMSet ret = { 0 };
    for (size_t i = 0; i < operations_size(ops); i++){
        ret.vs[operations_get(ops, i)]++;
    }
    return ret;
}

/* Adiciona dois multi sets */
void op_mset_add(OperationMSet* mset1, const OperationMSet* mset2){
    for (size_t i = 0; i < OPERATION_AMOUNT; i++){
        mset1->vs[i] += mset2->vs[i];
    }
}

/* Subtrai dois multi sets */
void op_mset_sub(OperationMSet* mset1, const OperationMSet* mset2){
    for (size_t i = 0; i < OPERATION_AMOUNT; i++){
        mset1->vs[i] -= mset2->vs[i];
    }
}

/* Verifica se a soma de dois multi set é menor ou igual a um outro multi set */
bool op_mset_lte(const OperationMSet* mset1, const OperationMSet* mset2, const OperationMSet* max){
    for (size_t i = 0; i < OPERATION_AMOUNT; i++){
        if(mset1->vs[i] + mset2->vs[i] > max->vs[i]){
            return false;
        }
    }
    return true;
}