#include "protocol.h"
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cerrno>

#define SERVER_NAME_LEN_MAX 255

#define DEBUG 1


typedef enum error_codes {
    OK = 0,
    ERROR = 1,
    TIMEOUT = 2,
} error_codes;

typedef enum calcMessage_message
{
    NOT = 0,
    RESULT_OK,
    RESULT_NOK
}calcMessage_message;


void print_calcMessage(calcMessage *message) {
#if DEBUG
    if (message == nullptr) {
        return;
    }
    printf("Type: %d\n", ntohs(message->type));
    printf("Message: %d\n", ntohl(message->message));
    printf("Protocol: %d\n", ntohs(message->protocol));
    printf("Major Version: %d\n", ntohs(message->major_version));
    printf("Minor Version: %d\n", ntohs(message->minor_version));
#endif
    if (message->message == htonl(RESULT_OK)) {
        printf("OK\n");
    } else {
        printf("ERROR\n");
    }

}

void create_calcMessage(calcMessage *message) {
    if (message == nullptr) {
        return;
    }

    memset(message, 0, sizeof(calcMessage));
    /*
     * type 22
       message 0
       protocol 17
       major_version 1
       minor_version 0
       Use network byte order for all fields
     * */

    message->type = htons(22);
    message->message = htonl(0);
    message->protocol = htons(17);
    message->major_version = htons(1);
    message->minor_version = htons(0);
}

error_codes send_message(int socket_fd, void *message, uint32_t message_len, struct sockaddr_in *server_address) {
    ssize_t sent_bytes;
    sent_bytes = sendto(socket_fd, message, message_len, 0, (struct sockaddr *) server_address,
                        sizeof(struct sockaddr_in));
    if (sent_bytes == -1) {
        perror("sendto");
        return ERROR;
    }
    return OK;
}

error_codes receive_message(int socket_fd, void *message, uint32_t message_len, struct sockaddr_in *server_address) {
    if (message == nullptr) {
        return ERROR;
    }
    memset(message, 0, message_len);

    if (server_address == nullptr) {
        return ERROR;
    }
    memset(server_address, 0, sizeof(struct sockaddr_in));

    ssize_t received_bytes;
    socklen_t server_address_len = sizeof(struct sockaddr_in);
    received_bytes = recvfrom(socket_fd, message, message_len, 0, (struct sockaddr *) server_address,
                              &server_address_len);
    if (received_bytes < 0) {
        // check for the timeout using errno
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return TIMEOUT;
        }
        perror("recvfrom");
        return ERROR;
    }

    return OK;
}

