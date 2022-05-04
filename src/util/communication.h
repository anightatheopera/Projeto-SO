#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "operations.h"

typedef struct {
    const char* filepath_in;
    const char* filepath_out;
    Operations* ops;
    int priority;
} Request;

#endif