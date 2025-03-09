#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Define der Server-Adresse
#define SERVER_ADDRESS "127.0.0.1:8080"

int main() {
    // Erstelle einen neuen Socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    // Konfiguriere den Server-Adresse und Port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr);
    server_addr.sin_port = htons(8080);

    // Binden des Serversockets an den Server-Adresse
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    // Listen auf Verbindungen
    if (listen(server_fd, 3) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Server läuft...\n");

    while (1) {
        // Warte auf Ereignisse mit epoll
        int events = epoll_wait(epoll_fd, epoll_events, 1, -1);

        if (events == -1) {
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i < events; i++) {
            struct epoll_event event = epoll_events[i];

            // Verbindung
            if (event.events & EPOLLIN) {
                printf("Verbindung aufgenommen...\n");

                // Lese Daten von der Verbindung
                char buffer[1024];
                int bytes_read = recv(server_fd, buffer, 1024, 0);
                if (bytes_read == -1) {
                    perror("recv");
                    continue;
                }

                printf("Empfangener Daten: %s\n", buffer);

                // Antwort auf das Client
                char* answer = "Hallo von Server!";
                send(server_fd, answer, strlen(answer), 0);
            }
        }
    }

    return 0;
}

// Epoll-Setup
int epoll_fd;
struct epoll_event {
    int events; // EPOLLIN | EPOLLOUT | EPollsERR
    int fd;     // Datei- oder Socket-Nummer
};
struct epoll_events {
    struct epoll_event events[1];
};

void init_epoll() {
    // Erstelle einen neuen Epoll-Server
    if ((epoll_fd = epoll_create(2)) == -1) {
        perror("epoll_create");
        exit(1);
    }

    // Registriere den Server-Socket
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT; // Registriere Verbindung und Datenempfang
    event.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl");
        exit(1);
    }
}
