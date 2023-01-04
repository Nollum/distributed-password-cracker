#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include "./lib/misc.h"

#define PORT "5555"
#define QUEUE_SIZE 100
#define MAX_LENGTH 10

extern const char ALPHABET[];
extern const int ALPHABET_SIZE;

int serv_socket;

void addClient(struct Client **head, struct sockaddr_storage client_address, socklen_t client_len, int clientID) {
    struct Client *new_client = (struct Client*)malloc(sizeof(struct Client));
    new_client->clientID = clientID;
    new_client->addr = client_address;
    new_client->addr_len = client_len;
    new_client->next = (*head);
    new_client->state = Free;
    (*head) = new_client;
}

struct Client *getClientByID(struct Client *head, int id) {

    struct Client *current = head;

    while (current != NULL) {
        if (current->clientID == id) return current;
        current = current->next;
    }
    return NULL;
}

struct Client *getClientByAddr(struct sockaddr *addr) {
    return NULL;
}

void removeClient(void *head, int clientID, int type) {

}

int sendJob(struct Client *head, char *hash, char *lower, char *upper, int worker_ctr) {
    struct Client *current = head;
    while (current != NULL && current->state == Busy) {
        current = current->next;
    }
    if (current == NULL) {
        return 0;
    }
    char buf[1024];
    char *data = (char *)calloc(strlen(lower)+strlen(upper)+3, sizeof(char));
    snprintf(data, strlen(hash)+strlen(lower)+strlen(upper)+3, "%s %s %s", hash, lower, upper);
    int msgLen = STDmsg(buf, sizeof(buf), JOB, current->clientID, data);
    int bytes_sent = sendto(serv_socket, 
                            buf, msgLen, 
                            0, 
                            (struct sockaddr*)(&(current->addr)), 
                            current->addr_len);
    if (bytes_sent < 1) {
        perror(strerror(errno));
        return 0;
    } 
    current->state = Busy;
    free(data);
    printf("Sent %d bytes: %s to %s\n", bytes_sent, lower, upper);
    return 1;
}

void shutdownWorkers(struct Client *head, int source_clientID) {
    struct Client *current = head;
    int msgLen;
    int bytes_sent;
    while (current != NULL) {
        char buf[1024];
        current->state = Free;
        if (current->clientID != source_clientID) {
            msgLen = STDmsg(buf, sizeof(buf), SHUTDOWN, current->clientID, NULL);
            bytes_sent = sendto(serv_socket, 
                        buf, msgLen, 
                        0, 
                        (struct sockaddr*)(&(current->addr)), 
                        current->addr_len);
            if (bytes_sent < 1) {
                perror(strerror(errno));
            }
            printf("Sent %d bytes\n", bytes_sent); 
        }
        current = current->next;
    }
}

int main() {
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *bind_address;
    if (getaddrinfo(NULL, PORT, &hints, &bind_address) != 0) {
        fprintf(stderr, "Could not configure local address\n");
        return 1;
    }
    printf("Creating server socket...\n");
    serv_socket = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (serv_socket < 0) {
        fprintf(stderr, "Could not create server socket\n");
        return 1;
    }
    printf("Binding socket to local address...\n");
    if (bind(serv_socket, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "Could not bind socket to local address\n");
        return 1;
    }
    freeaddrinfo(bind_address);
    fd_set master_reads;
    fd_set master_writes;
    FD_ZERO(&master_reads);
    FD_ZERO(&master_writes);
    FD_SET(serv_socket, &master_reads);
    FD_SET(serv_socket, &master_writes);
    FD_SET(fileno(stdout), &master_writes);
    int max_socket = serv_socket;
    printf("Waiting for connections...\n");

    srand(time(NULL));
    
    struct timeval tv;
    int randomId;

    struct Client *head_req_client = NULL;
    int req_client_ctr = 0;
    struct Client *head_worker_client = NULL;
    int worker_client_ctr = 0;
    struct Queue *jobQueue;

    jobQueue = createQueue(QUEUE_SIZE);
    
    struct Job *currentJob = NULL;
    int workers_pending = 0;

    while(1) {

        fd_set reads, writes;
        reads = master_reads;
        writes = master_writes;

        int res = select(serv_socket + 1, &reads, &writes, 0, 0);
        if (!res) {
            fprintf(stderr, "Server timed out\n");
            return 1;
        } else if (res < 0) {
            fprintf(stderr, "Error using select()\n");
            return 1;
        }

        struct Message *msg = NULL;
        struct sockaddr_storage client_address = {};
        socklen_t client_len = sizeof(client_address);

        if (FD_ISSET(serv_socket, &reads)) {
            char read[1024];
            memset(read, 0, sizeof(read));
            int bytes_received = recvfrom(serv_socket, read, sizeof(read), 0, (struct sockaddr*)&client_address, &client_len);
            if (bytes_received < 1) {
                fprintf(stderr, "connection closed\n");
                return 1;
            }
            msg = parseMessage(read);
        }

        if (msg) 
        {
            char buf[1024];
            int msgLen, bytes_sent;
            switch (msg->cmd)
            {
            case HASH:
                printf("New job received\n");
                randomId = rand() % (999999) + 1;
                addClient(&head_req_client, client_address, client_len, randomId);
                req_client_ctr++;
                struct Job *newJob = (struct Job*)malloc(sizeof(struct Job));
                newJob->clientID = randomId; 
                newJob->hashLength = 1;
                memset(newJob->hash, '\0', 1000);
                strncpy(newJob->hash, msg->data, strlen(msg->data));
                if (!isFull(jobQueue)) enqueue(jobQueue, newJob);
                break;
            case REQUEST_TO_JOIN:
                printf("New request to join\n");
                randomId = rand() % (999999) + 1;
                addClient(&head_worker_client, client_address, client_len, randomId);
                worker_client_ctr++;
                break;
            case ACK_JOB:
                break;
            case DONE_FOUND:
                msgLen = STDmsg(buf, sizeof(buf), DONE_FOUND, currentJob->clientID, msg->data);
                bytes_sent = sendto(serv_socket, 
                                    buf, msgLen, 
                                    0, 
                                    (struct sockaddr*)(&(getClientByID(head_req_client, currentJob->clientID)->addr)), 
                                    getClientByID(head_req_client, currentJob->clientID)->addr_len);
                if (bytes_sent < 1) {
                    perror(strerror(errno));
                }
                printf("Sent %d bytes\n", bytes_sent); 
                currentJob = NULL;
                getClientByID(head_worker_client, msg->clientID)->state = Free; 
                shutdownWorkers(head_worker_client, msg->clientID);
                break;
            case DONE_NOT_FOUND:
                getClientByID(head_worker_client, msg->clientID)->state = Free;
                if (currentJob != NULL && currentJob->lastChar >= ALPHABET_SIZE - 1) {
                    currentJob->hashLength++;
                    currentJob->lastChar = 0;
                }
                break;
            default:
                printf("Invalid command\n");
                break;
        }
        free(msg);

        }

        if (currentJob == NULL) currentJob = dequeue(jobQueue);

        if (FD_ISSET(serv_socket, &writes) ) {
            if (currentJob && worker_client_ctr > 0) {
                char lower[MAX_LENGTH];
                char upper[MAX_LENGTH];
                memset(lower, '\0', MAX_LENGTH);
                memset(upper, '\0', MAX_LENGTH);
                int interval = (int)ceil((double)(ALPHABET_SIZE-1) / (double)worker_client_ctr);
                for (int i = 0; i < currentJob->hashLength; i++) {
                    lower[i] = ALPHABET[currentJob->lastChar];
                    if (currentJob->lastChar+interval >= ALPHABET_SIZE) {
                        upper[i] = ALPHABET[ALPHABET_SIZE-1];
                    } else {
                        upper[i] = ALPHABET[currentJob->lastChar+interval];
                    }
                };
                int resp = sendJob(head_worker_client, currentJob->hash, lower, upper, worker_client_ctr);
                if (resp) currentJob->lastChar = currentJob->lastChar + interval;
            } 
        }
    }

    free(head_worker_client);
    free(head_req_client);
    free(jobQueue);
    close(serv_socket);

    return 0;
}