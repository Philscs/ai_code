#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_MESSAGE_SIZE 256
#define MAX_ROUTES 10
#define LOG_FILE "ipc_router.log"

// Structs
typedef struct {
    char destination[50];
    char payload[MAX_MESSAGE_SIZE];
} Message;

typedef struct {
    char route[50];
    void (*handler)(const Message*);
} Route;

// Globals
static Route routes[MAX_ROUTES];
static int route_count = 0;
static pthread_mutex_t route_mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE* log_file = NULL;

// Logging
void log_message(const char* level, const char* message) {
    if (!log_file) {
        log_file = fopen(LOG_FILE, "a");
        if (!log_file) {
            fprintf(stderr, "Failed to open log file\n");
            return;
        }
    }
    fprintf(log_file, "[%s] %s\n", level, message);
    fflush(log_file);
}

// Message Validation
bool validate_message(const Message* msg) {
    if (!msg || strlen(msg->destination) == 0 || strlen(msg->payload) == 0) {
        log_message("ERROR", "Invalid message received");
        return false;
    }
    return true;
}

// Route Management
bool add_route(const char* route, void (*handler)(const Message*)) {
    pthread_mutex_lock(&route_mutex);

    if (route_count >= MAX_ROUTES) {
        log_message("ERROR", "Maximum route limit reached");
        pthread_mutex_unlock(&route_mutex);
        return false;
    }

    strncpy(routes[route_count].route, route, sizeof(routes[route_count].route) - 1);
    routes[route_count].handler = handler;
    route_count++;

    log_message("INFO", "Route added successfully");
    pthread_mutex_unlock(&route_mutex);
    return true;
}

void remove_route(const char* route) {
    pthread_mutex_lock(&route_mutex);

    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].route, route) == 0) {
            routes[i] = routes[--route_count]; // Overwrite and decrement
            log_message("INFO", "Route removed successfully");
            pthread_mutex_unlock(&route_mutex);
            return;
        }
    }

    log_message("WARNING", "Route not found");
    pthread_mutex_unlock(&route_mutex);
}

// Message Routing
void route_message(const Message* msg) {
    if (!validate_message(msg)) {
        log_message("ERROR", "Message validation failed");
        return;
    }

    pthread_mutex_lock(&route_mutex);

    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].route, msg->destination) == 0) {
            routes[i].handler(msg);
            pthread_mutex_unlock(&route_mutex);
            return;
        }
    }

    log_message("WARNING", "No route found for message destination");
    pthread_mutex_unlock(&route_mutex);
}

// Example Handlers
void example_handler(const Message* msg) {
    printf("Message handled by example_handler: %s\n", msg->payload);
    log_message("INFO", "example_handler executed successfully");
}

// Resource Cleanup
void cleanup_resources() {
    pthread_mutex_destroy(&route_mutex);
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

// Main Function
int main() {
    // Initialize system
    log_message("INFO", "IPC Message Router started");

    // Add routes
    add_route("example", example_handler);

    // Simulate message routing
    Message msg = {"example", "This is a test message"};
    route_message(&msg);

    // Cleanup
    cleanup_resources();

    return 0;
}
