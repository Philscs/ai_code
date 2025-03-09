#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd, epoll_fd, event_count;
    struct sockaddr_in server_addr, client_addr;
    struct epoll_event event, events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // Bind server socket to address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("Failed to listen for connections");
        exit(EXIT_FAILURE);
    }

    // Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Failed to create epoll instance");
        exit(EXIT_FAILURE);
    }

    // Add server socket to epoll
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("Failed to add server socket to epoll");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Wait for events
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1) {
            perror("Failed to wait for events");
            exit(EXIT_FAILURE);
        }

        // Handle events
        for (int i = 0; i < event_count; i++) {
            if (events[i].data.fd == server_fd) {
                // Accept new connection
                socklen_t client_addr_len = sizeof(client_addr);
                client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_fd == -1) {
                    perror("Failed to accept connection");
                    exit(EXIT_FAILURE);
                }

                // Add client socket to epoll
                event.events = EPOLLIN;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    perror("Failed to add client socket to epoll");
                    exit(EXIT_FAILURE);
                }

                printf("New connection accepted\n");
            } else {
                // Receive data from client
                int bytes_received = recv(events[i].data.fd, buffer, BUFFER_SIZE, 0);
                if (bytes_received == -1) {
                    perror("Failed to receive data from client");
                    exit(EXIT_FAILURE);
                } else if (bytes_received == 0) {
                    // Client disconnected
                    printf("Client disconnected\n");
                    close(events[i].data.fd);
                } else {
                    // Process received data
                    printf("Received data from client: %.*s\n", bytes_received, buffer);

                    // Echo back to client
                    if (send(events[i].data.fd, buffer, bytes_received, 0) == -1) {
                        perror("Failed to send data to client");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }

    // Close server socket
    close(server_fd);

    return 0;
}
