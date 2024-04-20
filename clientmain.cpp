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
} error_codes_t;

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


error_codes process_calc_control(calcProtocol *server_calcProtocol, calcProtocol *client_response) {
    if (server_calcProtocol == nullptr || client_response == nullptr) {
        return ERROR;
    }

    memset(client_response, 0, sizeof(calcProtocol));


    client_response->type = htons(2);
    client_response->major_version = htons(1);
    client_response->minor_version = htons(0);
    client_response->id = htonl(ntohl(server_calcProtocol->id));
    client_response->inValue1 = htonl(ntohl(server_calcProtocol->inValue1));
    client_response->inValue2 = htonl(ntohl(server_calcProtocol->inValue2));
    client_response->flValue1 = server_calcProtocol->flValue1;
    client_response->flValue2 = server_calcProtocol->flValue2;
    client_response->arith = htonl(ntohl(server_calcProtocol->arith));

    int val1 = ntohl(server_calcProtocol->inValue1);
    int val2 = ntohl(server_calcProtocol->inValue2);
    int arith = ntohl(server_calcProtocol->arith);
    double result = 0;
    switch (arith) {
        case ADD: {
            result = val1 + val2;
            printf("ASSIGNMENT: %s %d %d\n", "add", val1, val2);
            break;
        }
        case SUB: {
            result = val1 - val2;
            printf("ASSIGNMENT: %s %d %d\n", "sub", val1, val2);
            break;
        }
        case MUL: {
            result = val1 * val2;
            printf("ASSIGNMENT: %s %d %d\n", "mul", val1, val2);
            break;
        }
        case DIV: {
            result = val1 / val2;
            printf("ASSIGNMENT: %s %d %d\n", "div", val1, val2);
            break;
        }
        case FADD: {
            result = server_calcProtocol->flValue1 + server_calcProtocol->flValue2;
            printf("ASSIGNMENT: %s %f %f\n", "fadd", server_calcProtocol->flValue1, server_calcProtocol->flValue2);
            break;
        }
        case FSUB: {
            result = server_calcProtocol->flValue1 - server_calcProtocol->flValue2;
            printf("ASSIGNMENT: %s %f %f\n", "fsub", server_calcProtocol->flValue1, server_calcProtocol->flValue2);
            break;
        }
        case FMUL: {
            result = server_calcProtocol->flValue1 * server_calcProtocol->flValue2;
            printf("ASSIGNMENT: %s %f %f\n", "fmul", server_calcProtocol->flValue1, server_calcProtocol->flValue2);
            break;
        }
        case FDIV: {
            result = server_calcProtocol->flValue1 / server_calcProtocol->flValue2;
            printf("ASSIGNMENT: %s %f %f\n", "fdiv", server_calcProtocol->flValue1, server_calcProtocol->flValue2);
            break;
        }
    }

    // Based on the operation update the result.
    if (arith >= ADD && arith <= DIV) {
        client_response->inResult = htonl((int32_t) result);
    } else {
        // float operations
        client_response->flResult = (float)result;
    }
#if 1
    // print the result
    printf("Result: %f\n", (float)result);
#endif
    return OK;
}


void print_calcProtcol(calcProtocol *message) {
#if DEBUG
    if (message == nullptr) {
        return;
    }
    printf("Type: %d\n", ntohs(message->type));
    printf("Major Version: %d\n", ntohs(message->major_version));
    printf("Minor Version: %d\n", ntohs(message->minor_version));
    printf("ID: %d\n", ntohl(message->id));
    printf("Arith: %d\n", ntohl(message->arith));
    printf("inValue1: %d\n", ntohl(message->inValue1));
    printf("inValue2: %d\n", ntohl(message->inValue2));
    printf("inResult: %d\n", ntohl(message->inResult));
    printf("flValue1: %f\n", message->flValue1);
    printf("flValue2: %f\n", message->flValue2);
    printf("flResult: %f\n", message->flResult);
#endif
}

int main(int argc, char *argv[]) {
    char server_name[SERVER_NAME_LEN_MAX + 1] = {0};
    int server_port, socket_fd;


    struct calcProtocol server_calcProtocol;
    struct sockaddr_in client_address;
    struct timeval tv;

    char delim[] = ":";
    char *Desthost = strtok(argv[1], delim);
    char *Destport = strtok(nullptr, delim);

    // Check if the input is correct
    if (Desthost == nullptr || Destport == nullptr) {
        printf("Usage: %s <ip>:<port>\n", argv[0]);
        exit(1);
    }

    /* Get server name from command line arguments or stdin. */
    strncpy(server_name, Desthost, SERVER_NAME_LEN_MAX);

    /* Get server port from command line arguments or stdin. */
    server_port = atoi(Destport);

    /* Print IP or Host and Port number */
    // Host 127.0.0.1, and port 5000.

    printf("Host %s, and port %d.\n", server_name, server_port);
    
    /* Get server host from server name. */
    struct addrinfo hints, *server_info, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Support both IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM;

    int dns_status = getaddrinfo(server_name, Destport, &hints, &server_info);
    if (dns_status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(dns_status));
        return 1;
    };


    // Extract the first IP address
    struct sockaddr_storage server_addr;
    for (p = server_info; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET || p->ai_family == AF_INET6) {
            memcpy(&server_addr, p->ai_addr, p->ai_addrlen);
            break;
        }
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to extract IP address\n");
        return 2;
    }

    // Convert the IP address to a string and print it
    char ipstr[INET6_ADDRSTRLEN];
    void *addr;
    if (server_addr.ss_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)&server_addr;
        addr = &(ipv4->sin_addr);
    } else {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&server_addr;
        addr = &(ipv6->sin6_addr);
    }

    inet_ntop(server_addr.ss_family, addr, ipstr, sizeof ipstr);
    printf("Connecting to %s\n", ipstr);

    /* Create UDP socket. */
    if ((socket_fd = socket(server_host->h_addrtype, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    /* Set Socket option for timeout */
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
    }

    // print the details of the port used to send the UDP message
    struct sockaddr_in local_address;
    socklen_t local_address_len = sizeof(local_address);
    if (getsockname(socket_fd, (struct sockaddr *) &local_address, &local_address_len) == -1) {
        perror("getsockname");
        exit(1);
    }
#if DEBUG
    printf("Local address: %s\n", inet_ntoa(local_address.sin_addr));
    printf("Local port: %d\n", ntohs(local_address.sin_port));
#endif


    calcMessage message;
    create_calcMessage(&message);

    // Send the message to the server
    bool response_received = false;
    int number_of_tries = 3;

    do {
        if (send_message(socket_fd, &message, sizeof(message), &server_address) != OK) {
            exit(1);
        }

        // Receive message from server and store it in message_received and server_address in client_address
        error_codes status = receive_message(socket_fd, &server_calcProtocol, sizeof(server_calcProtocol),
                                             &client_address);
        if (status == OK) {
            response_received = true;
        } else if (status == TIMEOUT) {
            printf("Timeout\n");
        } else {
            exit(1);
        }
    } while (!response_received && --number_of_tries > 0);


    if (!response_received) {
        printf("No response received\n");
        exit(1);
    }


    // process the calcProtocol message received from the server
    calcProtocol client_response;
    process_calc_control(&server_calcProtocol, &client_response);


    // print the message received and the client address
    print_calcProtcol(&server_calcProtocol);
#if DEBUG
    printf("Client address: %s\n", inet_ntoa(client_address.sin_addr));
#endif
    print_calcProtcol(&client_response);


    // Receive CalcMessage from the server
    calcMessage server_calcMessage;


    // Setup Retries
    response_received = false;
    number_of_tries = 3;
    do
    {
        // Send the response to the server
        if (send_message(socket_fd, &client_response, sizeof(client_response), &server_address) != OK) {
            exit(1);
        }

        // Receive message from server and store it in message_received and server_address in client_address
        error_codes status  = receive_message(socket_fd, &server_calcMessage, sizeof(server_calcMessage),
                                             &client_address);

        if (status == OK) {
            response_received = true;
        } else if (status == TIMEOUT) {
            printf("Timeout\n");
        } else {
            exit(1);
        }
    } while (!response_received && --number_of_tries > 0);

    if (!response_received) {
        printf("No response received\n");
        exit(1);
    }

    // print the message received and the client address
    print_calcMessage(&server_calcMessage);

}
