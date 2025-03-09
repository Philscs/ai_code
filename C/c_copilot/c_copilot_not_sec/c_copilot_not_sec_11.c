#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_REQUEST_SIZE 1024

void handle_request(int client_socket) {
    char request[MAX_REQUEST_SIZE];
    memset(request, 0, sizeof(request));

    // Read the request from the client
    read(client_socket, request, sizeof(request));

    // Parse the request method
    char method[10];
    sscanf(request, "%s", method);

    // Handle different request methods
    if (strcmp(method, "GET") == 0) {
        // Handle GET request
        // ...
    } else if (strcmp(method, "POST") == 0) {
        // Handle POST request
        // ...
    } else if (strcmp(method, "PUT") == 0) {
        // Handle PUT request
        // ...
    } else if (strcmp(method, "DELETE") == 0) {
        // Handle DELETE request
        // ...
    } else {
        // Invalid request method
        char response[] = "HTTP/1.1 400 Bad Request\r\n\r\n";
        write(client_socket, response, sizeof(response));
    }

    // Close the client socket
    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length = sizeof(client_address);

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Bind the socket to the server address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Failed to bind socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("Failed to listen for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        // Accept a client connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);
        if (client_socket < 0) {
            perror("Failed to accept client connection");
            exit(EXIT_FAILURE);
        }

        // Handle the client request
        handle_request(client_socket);
    }

    // Close the server socket
    close(server_socket);

    return 0;
}
