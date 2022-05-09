#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "operations.h"

typedef enum {
    REQUEST_STATUS,
    REQUEST_OPERATIONS
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
    size_t requests_being_processed;
} ServerMessageStatus;

typedef struct {
    ServerMessageType type;
} ServerMessage;

#endif