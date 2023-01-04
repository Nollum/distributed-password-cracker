#ifndef MISC_H
#define MISC_H

#include "commands.h"
#include <sys/socket.h>

#define PROTOCOL_ID 1150

typedef enum {
    Busy,
    Free
} State;

struct Job {
    char hash[1000];
    char result[1000];
    int lastChar;
    int hashLength;
    int clientID;
};

struct Client {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int clientID;
    State state;
    struct Client *next;
};

struct Message {
    command cmd;
    int clientID;
    char *data;
};

struct Queue {
    int front, rear, size;
    unsigned capacity;
    struct Job **array;
};

int STDmsg(char *buf, size_t size, command cmd, const int clientID, const char *varData);

struct Message *parseMessage(char *message);

struct Queue *createQueue(unsigned capacity);

int isFull(struct Queue *queue);

int isEmpty(struct Queue *queue);

void enqueue(struct Queue *queue, struct Job *job);

struct Job *dequeue(struct Queue *queue);

void *front(struct Queue *queue);

void *rear(struct Queue *queue);

#endif