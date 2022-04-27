#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdbool.h>

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

bool str_to_operation(const char* str, Operation* op);
const char* operation_to_str(Operation op);

typedef struct operations Operations;

Operations* operations_new();
bool operations_add(Operations* ops, Operation op);
Operation operations_get(Operations* ops, size_t i);
size_t operations_size(Operations* ops);
void operations_free(Operations* ops);


#endif