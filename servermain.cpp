#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdbool.h>
#include "protocol.h"
#include "calcLib.h"


int get_server_address_info(const char *server_name, const char *Destport, struct sockaddr_storage *server_addr);

void print_server_ip_addr(const struct sockaddr_storage *server_addr);

int create_server_socket(int port, struct sockaddr_storage *server_address);


// Constants
#define BUFFER_SIZE 1024
#define MAX_TIMEOUT_IN_SEC 10
#define SERVER_NAME_LEN_MAX 255
#define MAX_CLIENTS 100
#define INVALID_CLIENT_ID 0

// Enum for operation mapping
enum operation {
    ADD = 1,
    SUB = 2,
    MUL = 3,
    DIV = 4,
    FADD = 5,
    FSUB = 6,
    FMUL = 7,
    FDIV = 8,
};

enum client_state {
    DISCONNECTED = 0,
    CONNECTED = 1,
    OPERATION_SENT = 2,
    RESULT_RECEIVED = 3,
    RESULT_SENT = 4,
    TIMEOUT = 5
};

struct assignment {
    enum operation operation;
    float fval[2];
    int ival[2];
    double result;
    bool is_float;
};

// Dictionary to track clients and their assignments
struct ClientAssignment {
    uint32_t id;
    time_t timestamp;
    struct sockaddr_in client_addr;
    enum client_state state;
    struct assignment operation;
};

struct ClientAssignment client_assignments[MAX_CLIENTS] = {{0}};

int create_new_operation(char *message, size_t message_len, struct assignment *operation) {
    unsigned int client_id = rand();

    // verify the input parameters
    if (message == NULL || operation == NULL || message_len == 0) {
        fprintf(stderr, "Invalid input parameters\n");
        return -1;
    }

    // Create a new calcProtocol message
    struct calcProtocol *calc_protocol = (struct calcProtocol *) message;

    // Generate random numbers
    double f1 = randomFloat();
    double f2 = randomFloat();

    int i1 = randomInt();
    int i2 = randomInt();

    // G    // Create a new calcProtocol messageenerate a random operation between 1 and 8

    enum operation op = (enum operation) (rand() % FDIV + 1);

    // Populate the calcProtocol message
    calc_protocol->type = htons(1);
    calc_protocol->major_version = htons(1);
    calc_protocol->minor_version = htons(0);
    calc_protocol->id = htonl(client_id);
    calc_protocol->arith = htonl(op);
    calc_protocol->inValue1 = htonl(i1);
    calc_protocol->inValue2 = htonl(i2);
    calc_protocol->flValue1 = f1;
    calc_protocol->flValue2 = f2;
    // Reset the result
    calc_protocol->inResult = 0;
    calc_protocol->flResult = 0.0;

    // Store the operation
    operation->operation = op;
    operation->fval[0] = f1;
    operation->fval[1] = f2;
    operation->ival[0] = i1;
    operation->ival[1] = i2;
    operation->result = 0.0;
    operation->is_float = op >= FADD ? true : false;

    // Compute the result
    switch (op) {
        case ADD:
            operation->result = i1 + i2;
            break;
        case SUB:
            operation->result = i1 - i2;
            break;
        case MUL:
            operation->result = i1 * i2;
            break;
        case DIV:
            operation->result = (int) (i1 / i2);
            break;
        case FADD:
            operation->result = f1 + f2;
            break;
        case FSUB:
            operation->result = f1 - f2;
            break;
        case FMUL:
            operation->result = f1 * f2;
            break;
        case FDIV:
            operation->result = f1 / f2;
            break;
        default:
            fprintf(stderr, "Invalid operation\n");
            return -1;
    }

    return client_id;
}

void process_incoming_message(int server_socket, char *buffer, int recv_len, struct sockaddr *client_addr,
                              struct ClientAssignment *clients) {

    // verify the input parameters
    if (buffer == NULL || client_addr == NULL || clients == NULL) {
        fprintf(stderr, "Invalid input parameters\n");
        return;
    }


    // Check if the client is already in the list
    int client_index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (memcmp(&clients[i].client_addr, client_addr, sizeof(struct sockaddr_in)) == 0) {
            client_index = i;
            break;
        }
    }

    // If the client is not in the list, add it
    if (client_index == -1) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].state == DISCONNECTED) {
                clients[i].id = INVALID_CLIENT_ID;
                clients[i].state = CONNECTED;
                clients[i].client_addr = *((struct sockaddr_in *) client_addr);
                clients[i].timestamp = time(NULL);
                client_index = i;
                break;
            }
        }
    }

    // Get the state for the client
    enum client_state state = clients[client_index].state;

    // If the client is connected, send the operation
    if (state == CONNECTED) {
        // Verify the protocol supported by the client
        struct calcMessage *calc_message = (struct calcMessage *) buffer;

        // verify the following fields
        /* type            22
        message         0
        protocol        17
        major_version   1
        minor_version   0 */

        // populate local calc message
        struct calcMessage local_calc_message;
        local_calc_message.type = htons(calc_message->type);
        local_calc_message.message = htonl(calc_message->message);
        local_calc_message.protocol = htons(calc_message->protocol);
        local_calc_message.major_version = htons(calc_message->major_version);
        local_calc_message.minor_version = htons(calc_message->minor_version);

        // Check for the fields
        if (local_calc_message.type == 22 && local_calc_message.message == 0 && local_calc_message.protocol == 17 &&
            local_calc_message.major_version == 1 && local_calc_message.minor_version == 0) {
            // Create a new operation
            char operation_buffer[BUFFER_SIZE] = {0};
            clients[client_index].id = create_new_operation(operation_buffer, BUFFER_SIZE,
                                                            &clients[client_index].operation);
            if (clients[client_index].id == -1) {
                fprintf(stderr, "Error creating new operation\n");
                return;
            }

            // Update the client state
            clients[client_index].state = OPERATION_SENT;

            // Send the operation to the client
            int bytes_sent = sendto(server_socket, operation_buffer, sizeof(operation_buffer), 0, client_addr,
                                    sizeof(struct sockaddr_in));
            if (bytes_sent == -1) {
                perror("Error sending message");
                exit(EXIT_FAILURE);
            }

        } else {
            struct calcMessage error_response;
            //, type=2, message=2, major_version=1, minor_version=0.
            error_response.type = htons(2);
            error_response.message = htons(2);
            error_response.protocol = htons(17);
            error_response.major_version = htons(1);
            error_response.minor_version = htons(0);

            // Send the error response
            int bytes_sent = sendto(server_socket, &error_response, sizeof(error_response), 0,
                                    (struct sockaddr *) &client_addr, sizeof(client_addr));
            if (bytes_sent == -1) {
                perror("Error sending message");
                exit(EXIT_FAILURE);
            }
            clients[client_index].state = DISCONNECTED;
        }
    } else if (clients[client_index].state == OPERATION_SENT) {
        // Verify the response from the client
        struct calcProtocol *calc_protocol = (struct calcProtocol *) buffer;
        
        // Verify the following fields
        /* type            2
         * major_version   1
         * minor_version   0
         * id              client_id
         * result          result
         */

        // Populate local calc protocol
        struct calcProtocol local_calc_protocol;
        local_calc_protocol.type = htons(calc_protocol->type);
        local_calc_protocol.major_version = htons(calc_protocol->major_version);
        local_calc_protocol.minor_version = htons(calc_protocol->minor_version);
        local_calc_protocol.id = htonl(calc_protocol->id);
        local_calc_protocol.inResult = htonl(calc_protocol->inResult);
        local_calc_protocol.flResult = calc_protocol->flResult;

        // Check for the fields

        if (local_calc_protocol.type == 2 && local_calc_protocol.major_version == 1 &&
            local_calc_protocol.minor_version == 0 && local_calc_protocol.id == clients[client_index].id) {
            // Update the client state
            clients[client_index].state = RESULT_RECEIVED;

            bool is_response_correct = false;

            // Check if the result is correct
            if (clients[client_index].operation.is_float) {
                // compute the absolute difference between the two floating point numbers
                double diff = clients[client_index].operation.result - local_calc_protocol.flResult;
                // convert to positive
                if (diff < 0) {
                    diff = -diff;
                }

                // check if the difference is less than 0.0001
                if (diff < 0.0001) {
                    // set the response correct
                    is_response_correct = true;
                } else {
                    printf("Float Value: %f, %f\n", clients[client_index].operation.result,
                           local_calc_protocol.flResult);
                    printf("Difference: %f\n", diff);
                }
            } else {
                if (clients[client_index].operation.result == local_calc_protocol.inResult) {
                    is_response_correct = true;
                } else {
                    printf("Int Value: %d, %d\n", clients[client_index].operation.result,
                           local_calc_protocol.inResult);
                }
            }
            

            if (is_response_correct) {

                // Send the result to the client
                struct calcMessage correct_response = {0};
                correct_response.type = htons(2);
                correct_response.message = htonl(1);
                correct_response.protocol = htons(17);
                correct_response.major_version = htons(1);
                correct_response.minor_version = htons(0);

                // Send the result to the client
                int bytes_sent = sendto(server_socket, &correct_response, sizeof(correct_response), 0,
                                        (const struct sockaddr *) &clients[client_index].client_addr,
                                        sizeof(struct sockaddr_in));
                if (bytes_sent == -1) {
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }

                // Update the client state

            } else {
                // Send the error response
                struct calcMessage error_response;
                //, type=2, message=2, major_version=1, minor_version=0.
                error_response.type = htons(2);
                error_response.message = htons(2);
                error_response.protocol = htons(17);
                error_response.major_version = htons(1);
                error_response.minor_version = htons(0);

                // Send the error response
                int bytes_sent = sendto(server_socket, &error_response, sizeof(error_response), 0,
                                        (const struct sockaddr *) &clients[client_index].client_addr,
                                        sizeof(struct sockaddr_in));
                if (bytes_sent == -1) {
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
            }
            clients[client_index].state = RESULT_SENT;
        }
    }

    if (clients[client_index].state == RESULT_SENT) {
        // transaction is complete, reset the client state
        // Clear the client memory
        memset(&clients[client_index], 0, sizeof(struct ClientAssignment));
        clients[client_index].state = DISCONNECTED;
    }

    // Check for the timeout; if the client is not responding for more than 10 seconds, reset the client state to DISCONNECTED
    time_t current_time = time(NULL);

    // Iterate through the clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (current_time - clients[i].timestamp > MAX_TIMEOUT_IN_SEC) {
            // transaction is timed out, reset the client state
            // Clear the client memory
            memset(&clients[i], 0, sizeof(struct ClientAssignment));
            clients[i].state = DISCONNECTED;
        }
    }
}

int main(int argc, char *argv[]) {

    // Check for the command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ip>:<port>\n", argv[0]);
        exit(1);
    }

    // init calc lib
    initCalcLib();

    char server_name[SERVER_NAME_LEN_MAX + 1] = {0};
    int server_port;

    struct sockaddr_storage server_address;

    char delim[] = ":";
    char *Desthost = strtok(argv[1], delim);
    char *Destport = strtok(NULL, delim);

    // Check if the input is correct
    if (Desthost == NULL || Destport == NULL) {
        printf("Usage: %s <ip>:<port>\n", argv[0]);
        exit(1);
    }

     // init calc lib
    initCalcLib();

    memset(client_assignments, 0, sizeof(client_assignments));

    /* Get server name from command line arguments or stdin. */
    strncpy(server_name, Desthost, SERVER_NAME_LEN_MAX);

    /* Get server port from command line arguments or stdin. */
    server_port = atoi(Destport);

    /* Print IP or Host and Port number */
    // Host 127.0.0.1, and port 5000.

    printf("Host %s, and port %d.\n", server_name, server_port);
    get_server_address_info(server_name, Destport, &server_address);
    print_server_ip_addr((struct sockaddr_storage *) &server_address);

    int server_socket = create_server_socket(server_port, &server_address);

    // Initialize the client assignments
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char receive_buffer[BUFFER_SIZE] = {0};

    while (1) {
        // Receive message from client
        ssize_t recv_len = recvfrom(server_socket, receive_buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr *) &client_addr,
                                    &client_addr_len);
        if (recv_len == -1) {
            perror("Error receiving message");
            exit(EXIT_FAILURE);
        }

        // handle the incoming message
        process_incoming_message(server_socket, receive_buffer, recv_len, (struct sockaddr *) &client_addr,
                                 client_assignments);

    }

    // Close the socket
    close(server_socket);

    return 0;
}
