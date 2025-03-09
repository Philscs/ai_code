#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define AUTH_USER "user"
#define AUTH_PASS "pass"

void handle_client(int client_socket);
int check_authentication(const char *auth_header);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d\n", PORT);

    // Accept and handle incoming connections
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Read request
    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        perror("Read failed");
        return;
    }
    buffer[bytes_read] = '\0';

    // Parse request line
    char method[16], path[256], version[16];
    sscanf(buffer, "%15s %255s %15s", method, path, version);

    // Check for Authorization header
    char *auth_header = strstr(buffer, "Authorization: Basic ");
    if (auth_header) {
        auth_header += strlen("Authorization: Basic ");
        char *newline = strchr(auth_header, '\n');
        if (newline) *newline = '\0';
    }

    if (!auth_header || !check_authentication(auth_header)) {
        const char *response = "HTTP/1.1 401 Unauthorized\r\n"
                               "WWW-Authenticate: Basic realm=\"Restricted\"\r\n"
                               "Content-Length: 0\r\n\r\n";
        write(client_socket, response, strlen(response));
        return;
    }

    // Handle GET and POST methods
    if (strcmp(method, "GET") == 0) {
        const char *response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 13\r\n\r\n"
                               "Hello, world!";
        write(client_socket, response, strlen(response));
    } else if (strcmp(method, "POST") == 0) {
        const char *response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 11\r\n\r\n"
                               "Post Success";
        write(client_socket, response, strlen(response));
    } else {
        const char *response = "HTTP/1.1 405 Method Not Allowed\r\n"
                               "Content-Length: 0\r\n\r\n";
        write(client_socket, response, strlen(response));
    }
}

int check_authentication(const char *auth_header) {
    char decoded[128];
    char expected[128];

    snprintf(expected, sizeof(expected), "%s:%s", AUTH_USER, AUTH_PASS);

    // Decode Base64 (simple approach, assuming single-line Base64 string)
    FILE *pipe = popen("base64 -d", "w+");
    if (!pipe) return 0;
    fprintf(pipe, "%s", auth_header);
    fflush(pipe);
    rewind(pipe);
    fread(decoded, 1, sizeof(decoded) - 1, pipe);
    decoded[strcspn(decoded, "\n")] = '\0';
    pclose(pipe);

    return strcmp(decoded, expected) == 0;
}
