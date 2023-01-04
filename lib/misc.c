#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "misc.h"
#include "commands.h"
#include <sys/socket.h>

const char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const int ALPHABET_SIZE = sizeof(ALPHABET) - 1;

int STDmsg(char *buf, size_t size, command cmd, const int clientID, const char *varData) {
    memset(buf, '\0', size);
    sprintf(buf + strlen(buf), "%d\r\n", PROTOCOL_ID);
    sprintf(buf + strlen(buf), "%d\r\n", cmd);
    sprintf(buf + strlen(buf), "%d\r\n", clientID);
    if (varData) {
        sprintf(buf + strlen(buf), "%s\r\n", varData);
    }
    sprintf(buf + strlen(buf), "\r\n");
    return strlen(buf);
}

struct Message *parseMessage(char *message) {
    char *token = strtok(message, "\r\n");
    int protocolID = atoi(token);
    if (protocolID == PROTOCOL_ID) {
        command cmd;
        int clientID;
        char *data;
        cmd = (command)atoi(strtok(NULL, "\r\n"));
        clientID = atoi(strtok(NULL, "\r\n"));
        data = strtok(NULL, "\r\n");
        struct Message *parsed = (struct Message*)malloc(sizeof(struct Message));
        parsed->clientID = clientID;
        parsed->cmd = cmd;
        parsed->data = data;
        return parsed;
    }
    return NULL;
}

struct Queue *createQueue(unsigned capacity) {
    struct Queue *queue = (struct Queue*)malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;
    queue->array = (struct Job**)malloc(sizeof(struct Job*) * capacity);
    return queue;
}

int isEmpty(struct Queue *queue) {
    return (queue->size == 0);
}

int isFull(struct Queue *queue) {
    return (queue->size == queue->capacity);
}

void enqueue(struct Queue *queue, struct Job *job) {
    if (isFull(queue)) return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = job;
    queue->size = queue->size + 1;
}

struct Job *dequeue(struct Queue *queue) {
    if (isEmpty(queue)) return NULL;
    struct Job *job = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return job;
}