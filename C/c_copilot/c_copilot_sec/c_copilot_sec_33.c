// BEGIN: Buffer Management
typedef struct {
    char* data;
    size_t size;
} Buffer;

Buffer* createBuffer(size_t size) {
    Buffer* buffer = malloc(sizeof(Buffer));
    buffer->data = malloc(size);
    buffer->size = size;
    return buffer;
}

void destroyBuffer(Buffer* buffer) {
    free(buffer->data);
    free(buffer);
}
// END: Buffer Management

// BEGIN: Error Handling
typedef enum {
    NO_ERROR,
    INVALID_PACKET,
    CONNECTION_ERROR,
    TIMEOUT_ERROR
} ErrorCode;

void handleError(ErrorCode error) {
    switch (error) {
        case INVALID_PACKET:
            printf("Invalid packet received\n");
            break;
        case CONNECTION_ERROR:
            printf("Connection error occurred\n");
            break;
        case TIMEOUT_ERROR:
            printf("Timeout error occurred\n");
            break;
        default:
            printf("Unknown error occurred\n");
            break;
    }
}
// END: Error Handling

// BEGIN: Security Validation
typedef struct {
    char* username;
    char* password;
} Credentials;

bool validateCredentials(Credentials* credentials) {
    // Perform security validation logic here
    return true; // Return true for demonstration purposes
}
// END: Security Validation

// BEGIN: Resource Management
typedef struct {
    int id;
    char* name;
} Resource;

Resource* allocateResource(char* name) {
    Resource* resource = malloc(sizeof(Resource));
    resource->id = generateUniqueId();
    resource->name = strdup(name);
    return resource;
}

void releaseResource(Resource* resource) {
    free(resource->name);
    free(resource);
}
// END: Resource Management

// BEGIN: Logging System
typedef enum {
    INFO,
    WARNING,
    ERROR
} LogLevel;

void logMessage(LogLevel level, const char* message) {
    switch (level) {
        case INFO:
            printf("[INFO] %s\n", message);
            break;
        case WARNING:
            printf("[WARNING] %s\n", message);
            break;
        case ERROR:
            printf("[ERROR] %s\n", message);
            break;
        default:
            printf("%s\n", message);
            break;
    }
}
// END: Logging System

int main() {
    // Usage examples
    Buffer* buffer = createBuffer(1024);
    handleError(INVALID_PACKET);
    
    Credentials credentials = { "username", "password" };
    bool isValid = validateCredentials(&credentials);
    
    Resource* resource = allocateResource("example");
    releaseResource(resource);
    
    logMessage(INFO, "This is an informational message");
    
    return 0;
}
