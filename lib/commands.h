#ifndef COMMANDS_H
#define COMMANDS_H
typedef enum {
    REQUEST_TO_JOIN,
    JOB,
    ACK_JOB,
    PING,
    DONE_NOT_FOUND,
    DONE_FOUND,
    NOT_DONE,
    SHUTDOWN,
    HASH
} command; 
#endif