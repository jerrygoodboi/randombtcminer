#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <jansson.h>

#define SERVER "15.204.102.129"  // CKPool Solo IP
#define PORT 3333
#define BTC_ADDR "bc1q4djnhdm90727e8hydtycw6jar7q7f5yzsxd2ye"

// Global block header (80 bytes)
unsigned char block_header[80] = {0};

// Function to convert hex string to byte array
void hex_to_bytes(const char *hex, unsigned char *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        sscanf(hex + (i * 2), "%2hhx", &bytes[i]);
    }
}

// Function to construct the block header
void construct_block_header(const char *version, const char *prevhash,
                            const char *merkle_root, const char *ntime, const char *nbits) {
    hex_to_bytes(version, block_header, 4);       // Version (Little Endian)
    hex_to_bytes(prevhash, block_header + 4, 32); // Previous Block Hash
    hex_to_bytes(merkle_root, block_header + 36, 32); // Merkle Root
    hex_to_bytes(ntime, block_header + 68, 4);    // Timestamp
    hex_to_bytes(nbits, block_header + 72, 4);    // Bits (Difficulty Target)
    memset(block_header + 76, 0, 4);              // Nonce (Set to 0)
}

// Function to process JSON messages
void process_json(const char *json_str) {
    json_t *root;
    json_error_t error;

    root = json_loads(json_str, 0, &error);
    if (!root) {
        fprintf(stderr, "JSON parse error: %s\n", error.text);
        return;
    }

    json_t *method = json_object_get(root, "method");
    if (method && json_is_string(method) && strcmp(json_string_value(method), "mining.notify") == 0) {
        json_t *params = json_object_get(root, "params");
        if (params && json_is_array(params) && json_array_size(params) > 5) {
            const char *prevhash = json_string_value(json_array_get(params, 1));
            const char *merkle_root = json_string_value(json_array_get(params, 2));
            const char *version = json_string_value(json_array_get(params, 6));
            const char *ntime = json_string_value(json_array_get(params, 7));
            const char *nbits = json_string_value(json_array_get(params, 8));

            if (prevhash && merkle_root && version && ntime && nbits) {
                printf("Constructing Block Header...\n");
                construct_block_header(version, prevhash, merkle_root, ntime, nbits);
                printf("Block Header Constructed!\n");
            }
        }
    }

    json_decref(root);
}

// Receive data and parse JSON messages
void receive_data(int sock) {
    char buffer[4096] = {0};
    int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        printf("Server Response:\n%s\n", buffer);

        // Split multiple JSON messages
        char *json_msg = strtok(buffer, "\n");
        while (json_msg) {
            process_json(json_msg);
            json_msg = strtok(NULL, "\n");
        }
    }
}

// Send data to the mining server
void send_data(int sock, const char *data) {
    send(sock, data, strlen(data), 0);
}

// Subscription request
void sub(int sock) {
    char msg[256];
    snprintf(msg, sizeof(msg), "{\"id\":1, \"method\":\"mining.subscribe\", \"params\":[]}\n");
    send_data(sock, msg);
    receive_data(sock);
}

// Authorization request
void auth(int sock) {
    char msg[256];
    snprintf(msg, sizeof(msg), "{\"id\":2, \"method\":\"mining.authorize\", \"params\":[\"%s\", \"x\"]}\n", BTC_ADDR);
    send_data(sock, msg);
    receive_data(sock);
}

// Create connection and initialize mining
void create() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("Socket error"); exit(1); }

    struct sockaddr_in server = { .sin_family = AF_INET, .sin_port = htons(PORT) };
    server.sin_addr.s_addr = inet_addr(SERVER);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        exit(1);
    }

    sub(sock);
    auth(sock);
    while(1){
        receive_data(sock);
}

}

int main() {
    create();
    return 0;
}
