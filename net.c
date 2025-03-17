#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <jansson.h>

#define SERVER "15.204.102.129"  // CKPool Solo IP
#define PORT 3333
#define BTC_ADDR "bc1q4djnhdm90727e8hydtycw6jar7q7f5yzsxd2ye"
char msg[256];

// Send data to the mining server
void send_data(int sock, const char *data) {
    send(sock, data, strlen(data), 0);
}

// Process a single JSON message
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
        if (params && json_is_array(params) && json_array_size(params) > 1) {
            json_t *prevhash = json_array_get(params, 1);
            if (prevhash && json_is_string(prevhash)) {
                printf("Previous Block Hash: %s\n", json_string_value(prevhash));
            }
        }
    }

    json_decref(root);
}

// Receive and parse JSON responses (handling multiple messages)
void receive_data(int sock) {
    char buffer[4096] = {0};
    int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        printf("Server Response:\n%s\n", buffer);

        // Split multiple JSON messages if they exist
        char *json_msg = strtok(buffer, "\n");
        while (json_msg) {
            process_json(json_msg);
            json_msg = strtok(NULL, "\n");
        }
    }
}

void sub(int sock) {
    snprintf(msg, sizeof(msg), "{\"id\":1, \"method\":\"mining.subscribe\", \"params\":[]}\n");
    send_data(sock, msg);
    receive_data(sock);
}

void auth(int sock) {
    snprintf(msg, sizeof(msg), "{\"id\":2, \"method\":\"mining.authorize\", \"params\":[\"%s\", \"x\"]}\n", BTC_ADDR);
    send_data(sock, msg);
    receive_data(sock);
}

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
}

int main() {
    create();
    return 0;
}
