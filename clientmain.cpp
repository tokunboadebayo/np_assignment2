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