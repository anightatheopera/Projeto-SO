#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdbool.h>

#define OPERATION_MSET_FMT "{ %d, %d, %d, %d, %d, %d, %d }"
#define OPERATION_MSET_ARG(mset) mset.vs[0], mset.vs[1], mset.vs[2], mset.vs[3], mset.vs[4], mset.vs[5], mset.vs[6]

typedef enum {
    BCOMPRESS,
    BDECOMPRESS,
    GCOMPRESS,
    GDECOMPRESS,
    ENCRYPT,
    DECRYPT,
    NOP,
    OPERATION_AMOUNT
} Operation;

typedef struct {
    size_t vs[OPERATION_AMOUNT];
} OperationMSet;


bool str_to_operation(const char* str, Operation* op);
const char* operation_to_str(Operation op);

typedef struct operations Operations;

Operations* operations_new();
bool operations_add(Operations* ops, Operation op);
Operation operations_get(Operations* ops, size_t i);
size_t operations_size(Operations* ops);
void operations_free(Operations* ops);
OperationMSet operations_to_mset(Operations* ops);

// current mset + request mset
// mset1 + mset2
void op_mset_add(OperationMSet* mset1, OperationMSet* mset2);

// current mset - request mset
// mset1 - mset2
void op_mset_sub(OperationMSet* mset1, OperationMSet* mset2);

// (current mset + request mset < max)? se sim, entao podemos correr o request 
// all mset1 + mset2 < max ?
bool op_mset_gt(OperationMSet* mset1, OperationMSet* mset2, OperationMSet* max);

#endif