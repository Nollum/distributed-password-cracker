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
#include "../lib/commands.h"
#include "../lib/misc.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Invalid usage: request_client hostname port hash\n");
        return 1;
    }
    const char *serveraddr = argv[1];
    const char *serverport = argv[2];
    const char *hash = argv[3];
    
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

    char buf[1024];
    int msgLen = STDmsg(buf, sizeof(buf), HASH, 0, hash);
    int bytes_sent = sendto(client_socket, 
                            buf, msgLen, 
                            0, server_address->ai_addr, server_address->ai_addrlen);
    printf("Sent %d bytes\n", bytes_sent);


    struct Message *msg = NULL;
    int clientID;
    char result[1024];
    memset(result, 0, sizeof(result));
    int bytes_received = recvfrom(client_socket, result, sizeof(result), 0, server_address->ai_addr, &server_address->ai_addrlen);
    if (bytes_received < 1) {
        fprintf(stderr, "connection closed\n");
        return 1;
    }
    msg = parseMessage(result);
    clientID = msg->clientID;

    switch (msg->cmd)
    {
    case DONE_FOUND:
        printf("Password found: %s\n", msg->data);
        break;
    case DONE_NOT_FOUND:
        printf("Password not found\n");
        break; 
    default:
        printf("Server response error\n");
    }

    freeaddrinfo(server_address);
    close(client_socket);

    return 0;
}