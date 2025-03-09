#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <base64.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_HEADERS 100
#define MAX_CLIENTS 10
#define USERNAME "admin"
#define PASSWORD "secret"

typedef struct {
    char *name;
    char *value;
} Header;

typedef struct {
    char method[16];
    char path[256];
    char version[16];
    Header headers[MAX_HEADERS];
    int header_count;
    char *body;
} HttpRequest;

typedef struct {
    int status_code;
    char *status_text;
    Header headers[MAX_HEADERS];
    int header_count;
    char *body;
} HttpResponse;

// Funktion zum Parsen der Request-Headers
void parse_headers(char *buffer, HttpRequest *request) {
    char *line = strtok(buffer, "\r\n");
    
    // Parse Request-Line
    sscanf(line, "%s %s %s", request->method, request->path, request->version);
    
    // Parse Headers
    request->header_count = 0;
    while ((line = strtok(NULL, "\r\n")) != NULL && strlen(line) > 0) {
        char *name = strtok(line, ":");
        char *value = strtok(NULL, "\r\n");
        
        if (name && value) {
            // Remove leading whitespace from value
            while (isspace(*value)) value++;
            
            request->headers[request->header_count].name = strdup(name);
            request->headers[request->header_count].value = strdup(value);
            request->header_count++;
        }
    }
}

// Funktion zum Erstellen der Response
void create_response(HttpRequest *request, HttpResponse *response) {
    // Überprüfe Basic Authentication
    int authorized = 0;
    for (int i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->headers[i].name, "Authorization") == 0) {
            char *auth_value = request->headers[i].value;
            if (strncmp(auth_value, "Basic ", 6) == 0) {
                char *credentials = base64_decode(auth_value + 6);
                char expected[256];
                snprintf(expected, sizeof(expected), "%s:%s", USERNAME, PASSWORD);
                if (strcmp(credentials, expected) == 0) {
                    authorized = 1;
                }
                free(credentials);
            }
        }
    }
    
    if (!authorized) {
        response->status_code = 401;
        response->status_text = "Unauthorized";
        response->header_count = 1;
        response->headers[0].name = "WWW-Authenticate";
        response->headers[0].value = "Basic realm=\"Restricted Area\"";
        response->body = "401 Unauthorized\n";
        return;
    }

    // Handle verschiedene Request Methods
    if (strcmp(request->method, "GET") == 0) {
        response->status_code = 200;
        response->status_text = "OK";
        response->body = "Hello World!\n";
    } else if (strcmp(request->method, "POST") == 0) {
        response->status_code = 201;
        response->status_text = "Created";
        response->body = "Resource created\n";
    } else if (strcmp(request->method, "PUT") == 0) {
        response->status_code = 200;
        response->status_text = "OK";
        response->body = "Resource updated\n";
    } else if (strcmp(request->method, "DELETE") == 0) {
        response->status_code = 204;
        response->status_text = "No Content";
        response->body = "";
    } else {
        response->status_code = 405;
        response->status_text = "Method Not Allowed";
        response->body = "405 Method Not Allowed\n";
    }

    // Setze Standard-Headers
    response->header_count = 3;
    response->headers[0].name = "Content-Type";
    response->headers[0].value = "text/plain";
    response->headers[1].name = "Date";
    
    time_t now = time(NULL);
    char date[100];
    strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    response->headers[1].value = strdup(date);
    
    // Keep-Alive Header
    int keep_alive = 0;
    for (int i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->headers[i].name, "Connection") == 0 &&
            strcasecmp(request->headers[i].value, "keep-alive") == 0) {
            keep_alive = 1;
            break;
        }
    }
    
    response->headers[2].name = "Connection";
    response->headers[2].value = keep_alive ? "keep-alive" : "close";
}

// Funktion zum Senden der Response
void send_response(int client_socket, HttpResponse *response) {
    char buffer[BUFFER_SIZE];
    
    // Sende Status-Line
    snprintf(buffer, sizeof(buffer), "HTTP/1.1 %d %s\r\n", 
             response->status_code, response->status_text);
    send(client_socket, buffer, strlen(buffer), 0);
    
    // Sende Headers
    for (int i = 0; i < response->header_count; i++) {
        snprintf(buffer, sizeof(buffer), "%s: %s\r\n",
                 response->headers[i].name, response->headers[i].value);
        send(client_socket, buffer, strlen(buffer), 0);
    }
    
    // Sende Body
    snprintf(buffer, sizeof(buffer), "Content-Length: %lu\r\n\r\n",
             strlen(response->body));
    send(client_socket, buffer, strlen(buffer), 0);
    send(client_socket, response->body, strlen(response->body), 0);
}

// Handler für Client-Verbindungen
void *handle_client(void *client_socket_ptr) {
    int client_socket = *((int *)client_socket_ptr);
    free(client_socket_ptr);
    
    char buffer[BUFFER_SIZE];
    int keep_alive = 1;
    
    while (keep_alive) {
        // Empfange Request
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) break;
        buffer[bytes_received] = '\0';
        
        // Parse Request
        HttpRequest request;
        parse_headers(buffer, &request);
        
        // Erstelle und sende Response
        HttpResponse response;
        create_response(&request, &response);
        send_response(client_socket, &response);
        
        // Überprüfe Keep-Alive
        keep_alive = 0;
        for (int i = 0; i < request.header_count; i++) {
            if (strcasecmp(request.headers[i].name, "Connection") == 0 &&
                strcasecmp(request.headers[i].value, "keep-alive") == 0) {
                keep_alive = 1;
                break;
            }
        }
        
        // Cleanup
        for (int i = 0; i < request.header_count; i++) {
            free(request.headers[i].name);
            free(request.headers[i].value);
        }
    }
    
    close(client_socket);
    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    
    // Erstelle Socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Setze Socket-Optionen
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Konfiguriere Server-Adresse
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Binde Socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Starte Listening
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server started on port %d\n", PORT);
    
    // Hauptloop
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        // Akzeptiere neue Verbindung
        int *client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (*client_socket < 0) {
            perror("Accept failed");
            free(client_socket);
            continue;
        }
        
        // Erstelle Thread für Client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_socket) != 0) {
            perror("Thread creation failed");
            close(*client_socket);
            free(client_socket);
            continue;
        }
        pthread_detach(thread_id);
    }
    
    close(server_socket);
    return 0;
}