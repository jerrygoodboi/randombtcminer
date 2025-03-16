#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER "149.56.27.225"  // CKPool Solo IP
#define PORT 3333
#define BTC_ADDR "bc1q4djnhdm90727e8hydtycw6jar7q7f5yzsxd2ye"

// Send data to the mining server
void send_data(int sock, const char *data) {
    send(sock, data, strlen(data), 0);
}

// Receive and print response
void receive_data(int sock) {
    char buffer[1024] = {0};
    int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        printf("Server: %s\n", buffer);
    }
}

int main() {
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("Socket error"); return 1; }

    struct sockaddr_in server = { .sin_family = AF_INET, .sin_port = htons(PORT) };
    server.sin_addr.s_addr = inet_addr(SERVER);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        return 1;
    }

    char msg[256];

    // Subscribe to mining server
    snprintf(msg, sizeof(msg),
        "{\"id\":1, \"method\":\"mining.subscribe\", \"params\":[]}\n");
    send_data(sock, msg);
    receive_data(sock); // Read subscribe response

    // Authorize mining worker (BTC address)
    snprintf(msg, sizeof(msg),
        "{\"id\":2, \"method\":\"mining.authorize\", \"params\":[\"%s\", \"x\"]}\n", BTC_ADDR);
    send_data(sock, msg);
    receive_data(sock); // Read authorize response

    // Listen for mining jobs (mining.notify)
    while (1) {
        receive_data(sock);
    }

    close(sock);
    return 0;
}

