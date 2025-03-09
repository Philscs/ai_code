#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024

// Strukt für die Verbindungsdaten
typedef struct {
    int connfd;
    char auth[16];
} ConnData;

// Strukt für die Anfrage
typedef struct {
    int method;
    char path[256];
    char headers[1024];
    char body[MAX_BUFFER_SIZE];
} Request;

// Funktion zur Verarbeitung einer Anfrage
void process_request(ConnData *conn, Request *req) {
    // Überprüfe das Authentifizierungskennwort
    if (strcmp(req->body, "basicspassword") != 0) {
        send(conn->connfd, "HTTP/1.1 401 Unauthorized\r\n", strlen("HTTP/1.1 401 
Unauthorized\r\n"), 0);
        return;
    }

    // Überprüfe die HTTP-Methode
    if (req->method == 'G') { // GET
        printf("GET %s\r\n", req->path);
        send(conn->connfd, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
        send(conn->connfd, "\r\n", strlen("\r\n"), 0);
    } else if (req->method == 'P') { // POST
        printf("POST %s\r\n", req->path);
        send(conn->connfd, "HTTP/1.1 201 Created\r\n", strlen("HTTP/1.1 201 Created\r\n"), 0);
        send(conn->connfd, "\r\n", strlen("\r\n"), 0);
    } else {
        send(conn->connfd, "HTTP/1.1 405 Method Not Allowed\r\n", strlen("HTTP/1.1 405 Method Not 
Allowed\r\n"), 0);
        return;
    }

    // Schließe die Verbindung
    close(conn->connfd);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    ConnData conn;

    // Erstelle den Server-Listenender
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // Binden des Server-Listenenders
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen auf Verbindungen
    listen(server_fd, 3);

    while (1) {
        // Akzeptiere eine Verbindung
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        printf("Verbindung akzeptiert\r\n");

        // Überprüfe die Authentifizierungskennwort
        send(client_fd, "Basic realm=\"My Realm\"\r\n", strlen("Basic realm=\"My Realm\"\\n"), 
0);

        conn.connfd = client_fd;
        conn.auth[0] = '\0';

        char buffer[MAX_BUFFER_SIZE];
        Request request;

        // Warte auf die Anfrage
        int bytes_received = recv(client_fd, buffer, MAX_BUFFER_SIZE, 0);
        if (bytes_received == -1) {
            perror("recv");
            continue;
        }

        // Parsen der Anfrage
        char method[2];
        char path[256];
        char headers[1024];
        char body[MAX_BUFFER_SIZE];

        sscanf(buffer, "GET /path HTTP/1.1", method, path);
        sscanf(buffer + strlen("GET /path HTTP/1.1"), "\r\nHost: localhost\r\n\r\n%s", headers, 
&body);

        // Speichern der Anfrage in der Struktur
        request.method = (method[0] == 'G') ? 'G' : 'P';
        strcpy(request.path, path);
        strcpy(request.headers, headers);
        strcpy(request.body, body);

        // Verarbeiten der Anfrage
        process_request(&conn, &request);
    }

    return 0;
}
