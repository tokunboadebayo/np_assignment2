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

