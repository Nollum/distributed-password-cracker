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
#include <pthread.h>
#include "../lib/commands.h"
#include "../lib/misc.h"

char *bruteforce(char *lower, char *upper, char *hash, char *salt);
void *thread_handler(void *vargp);

extern const char ALPHABET[];
extern const int ALPHABET_SIZE;

struct thread_args {
    char *hash;
    char *lower;
    char *upper;
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Invalid usage: worker_client hostname port\n");
        return 1;
    }
    const char *serveraddr = argv[1];
    const char *serverport = argv[2];
    
    struct addrinfo hints;
    struct addrinfo *server_address;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(serveraddr, serverport, &hints, &server_address)) {
        fprintf(stderr, "Could not obtain address\n");
        return 1;
    }
    int client_socket;
    client_socket = socket(server_address->ai_family, server_address->ai_socktype, server_address->ai_protocol);
    if (client_socket < 0) {
        fprintf(stderr, "Could not create socket\n");
        return 1;
    }

    pthread_t thread_id;
    int running = 0;

    char buf[1024];
    int msgLen = STDmsg(buf, sizeof(buf), REQUEST_TO_JOIN, 0, NULL);
    int bytes_sent = sendto(client_socket, 
                            buf, msgLen, 
                            0, server_address->ai_addr, server_address->ai_addrlen);
    printf("Sent %d bytes\n", bytes_sent);

    int clientID;


    while(1) {

        if (running) {
            char buf[1024];
            void *retval;
            pthread_join(thread_id, &retval);
            if (retval) {
                printf("%s\n", (char *)retval);
                int msgLen = STDmsg(buf, sizeof(buf), DONE_FOUND, clientID, (char *)retval);
                int bytes_sent = sendto(client_socket, 
                                        buf, msgLen, 
                                        0, server_address->ai_addr, server_address->ai_addrlen);
                printf("Sent %d bytes\n", bytes_sent);
            } else {
                int msgLen = STDmsg(buf, sizeof(buf), DONE_NOT_FOUND, clientID, NULL);
                int bytes_sent = sendto(client_socket, 
                                        buf, msgLen, 
                                        0, server_address->ai_addr, server_address->ai_addrlen);
                printf("Sent %d bytes\n", bytes_sent);
            }
            running = 0;
        }

        struct Message *msg = NULL;

        char read[1024];
        memset(read, 0, sizeof(read));
        int bytes_received = recvfrom(client_socket, read, 1024, 0, server_address->ai_addr, &server_address->ai_addrlen);
        if (bytes_received < 1) {
            fprintf(stderr, "connection closed\n");
            return 1;
        }

        msg = parseMessage(read);

        if (msg->cmd == JOB) {
            clientID = msg->clientID;
            struct thread_args data;
            char *hash = strtok(msg->data, " ");
            char *lower = strtok(NULL, " ");
            char *upper = strtok(NULL, "\r\n");
            data.lower = lower;
            data.upper = upper;
            data.hash = hash;
            running = 1;
            pthread_create(&thread_id, NULL, thread_handler, (void *)&data);
        } else if (msg->cmd == SHUTDOWN) {
            printf("Shutdown request received\n");
            pthread_cancel(thread_id);
        }
        free(msg);
    }

    freeaddrinfo(server_address);
    close(client_socket);

    return 0;
}

void *thread_handler(void *vargp) {
    struct thread_args *data = (struct thread_args*)vargp;
    char salt[2];
    char *str;
    // for (int f = 0; f < ALPHABET_SIZE; f++) {
    //     for (int s = 0; s < ALPHABET_SIZE; s++) {
    //         salt[0] = ALPHABET[f];
    //         salt[1] = ALPHABET[s];
    //         str = bruteforce(data->lower, data->upper, data->hash, salt);
    //         if (str) {
    //             pthread_exit((void *)str);
    //         }
    //     }
    // } 
    str = bruteforce(data->lower, data->upper, data->hash, "AA");
    if (str) {
        pthread_exit((void *)str);
    }
    pthread_exit(NULL);
}

char *bruteforce(char *lower, char *upper, char *hash, char *salt) {

    char *str = (char *)malloc(strlen(lower)+1);
    memset(str, '\0', strlen(lower)+1);
    int indices[1000];
    int aIndex, len, i, j;
    strncpy(str, lower, strlen(lower));
    len = strlen(str);

    for (i = 0; i < len; i++) {
        for (j = 0; j < ALPHABET_SIZE; j++) {
            if (str[i] == ALPHABET[j]) {
                indices[i] = j;
            }
        }
    }

    int headIndex, carry;
    headIndex = len - 1;
    carry = 0;
    while (1) {
        for (i = 0; i < len; i++) {
            str[i] = ALPHABET[indices[i]];
        }

        // printf("%s\n", str);

        if (strcmp(crypt(str, salt), hash) == 0) {
            return str;
        } 

        if (strcmp(str, upper) == 0) {
            break;
        }
        headIndex = len - 1;
        indices[headIndex]++;
        if (indices[headIndex] == ALPHABET_SIZE) {
            indices[headIndex] = ALPHABET[indices[headIndex]] % ALPHABET_SIZE;
            headIndex--;
            carry = 1;
        }
        while (carry == 1) {
            indices[headIndex]++;
            if (indices[headIndex] == ALPHABET_SIZE) {
                indices[headIndex] = ALPHABET[indices[headIndex]] % ALPHABET_SIZE;
                headIndex--;
                carry = 1;
            } else {
                carry = 0;
            }
        }
    }
    return NULL;
}