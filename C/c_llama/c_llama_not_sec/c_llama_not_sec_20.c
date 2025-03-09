#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024
#define SEND_INTERVAL 10 // ms

// Congestion Control-Algorithmen
enum {
    CUBIC,
    FALSTIN,
    NYquist
};

int cubic_congestion_control(int current_rtt, int current_srtt) {
    return (current_rtt * current_rtt / current_srtt);
}

int falstin_congestion_control(int current_rtt, int current_srtt) {
    if (current_srtt == 0)
        return 3; // default value
    else
        return (1 + (current_rtt - current_srtt)) * (current_rtt + current_srtt) / (current_rtt - 
current_srtt);
}

int nyquist_congestion_control(int current_rtt, int current_srtt) {
    if (current_rtt == 0)
        return 3; // default value
    else
        return (1 + (current_rtt - current_srtt)) * (current_rtt + current_srtt) / (current_rtt - 
current_srtt);
}

// Flow Control-Algorithmen
int send_buffer(int buffer_size, int window_size) {
    if (buffer_size < window_size)
        return 0;
    else
        return 1;
}

void sender_main() {
    struct sockaddr_in server_addr, local_addr;
    socklen_t server_len = sizeof(server_addr);
    socklen_t client_len = sizeof(local_addr);
    int server_socket, client_socket;
    int send_buffer_size = MAX_BUFFER_SIZE * 2; // example value
    int window_size = 10000; // example value

    // Erstellen Sie einen neuen TCP-Connection
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // Binden Sie den Socket an eine Adresse
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); // Beispielport
    inet_pton(AF_INET, "127.0.0.1", &(server_addr.sin_addr));
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    // Listen Sie auf verfügbare Verbindungen
    listen(server_socket, 5);

    printf("Server-Adresse: %s:%d\n", inet_ntoa(server_addr.sin_addr), 
ntohs(server_addr.sin_port));

    // Erstellen Sie einen neuen Client-Connection
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // Binden Sie den Socket an eine Adresse
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(8081); // Beispielport
    inet_pton(AF_INET, "127.0.0.1", &(local_addr.sin_addr));
    if (bind(client_socket, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    // Verbinden Sie den Client mit dem Server
    connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Client-Adresse: %s:%d\n", inet_ntoa(local_addr.sin_addr), 
ntohs(local_addr.sin_port));

    // Senden von Daten
    int buffer_size = send_buffer(send_buffer_size, window_size);
    if (!buffer_size) {
        perror("Error sending data");
        exit(1);
    }

    char* data = "Hello, Server!";
    int bytes_sent = 0;
    while (bytes_sent < strlen(data)) {
        int packet_size = min(window_size - bytes_sent, send_buffer_size - bytes_sent);
        if (!send(packet_size)) {
            perror("Error sending data");
            exit(1);
        }
        printf("Sending packet of size %d...\n", packet_size);
        // Simulate Wait-zeit
        usleep(SEND_INTERVAL * 1000);
        bytes_sent += packet_size;
    }

    // Beenden Sie die Verbindung
    close(client_socket);
}

void receiver_main() {
    struct sockaddr_in server_addr, local_addr;
    socklen_t server_len = sizeof(server_addr);
    int server_socket;
    char buffer[MAX_BUFFER_SIZE];

    // Erstellen Sie einen neuen TCP-Connection
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // Binden Sie den Socket an eine Adresse
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); // Beispielport
    inet_pton(AF_INET, "127.0.0.1", &(server_addr.sin_addr));
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    // Listen Sie auf verfügbare Verbindungen
    listen(server_socket, 5);

    printf("Server-Adresse: %s:%d\n", inet_ntoa(server_addr.sin_addr), 
ntohs(server_addr.sin_port));

    // Erhalten Sie Daten von dem Server
    int bytes_received = recv(server_socket, buffer, MAX_BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        perror("Error receiving data");
        exit(1);
    }

    printf("Received: %s\n", buffer);

    // Beenden Sie die Verbindung
    close(server_socket);
}

int main() {
    sender_main();
    receiver_main();

    return 0;
}
