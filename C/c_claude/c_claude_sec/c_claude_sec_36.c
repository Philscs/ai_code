#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_MSG_SIZE 1024
#define MAX_LOG_SIZE 256
#define MAGIC_NUMBER 0xDEADBEEF

// Strukturen für Message Handling
typedef struct {
    unsigned int magic;          // Magic Number zur Validierung
    unsigned int msg_type;       // Typ der Nachricht
    unsigned int msg_size;       // Größe der Nachricht
    unsigned int checksum;       // Prüfsumme
    char payload[MAX_MSG_SIZE];  // Nutzdaten
} Message;

typedef struct {
    int msg_queue_id;           // Message Queue ID
    int allocated_resources;    // Anzahl allokierter Ressourcen
    FILE* log_file;            // Log-Datei Handle
} ProcessHandler;

// Logger Funktion
void log_event(ProcessHandler* handler, const char* message, const char* severity) {
    if (!handler->log_file) return;
    
    time_t now;
    time(&now);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';  // Entferne Newline
    
    fprintf(handler->log_file, "[%s] %s: %s\n", timestamp, severity, message);
    fflush(handler->log_file);
}

// Berechne Prüfsumme für Nachrichtenvalidierung
unsigned int calculate_checksum(const Message* msg) {
    unsigned int sum = 0;
    const unsigned char* data = (const unsigned char*)msg->payload;
    
    for (unsigned int i = 0; i < msg->msg_size; i++) {
        sum += data[i];
    }
    return sum;
}

// Initialisiere den Process Handler
ProcessHandler* init_process_handler(const char* log_file_path) {
    ProcessHandler* handler = (ProcessHandler*)malloc(sizeof(ProcessHandler));
    if (!handler) {
        perror("Failed to allocate handler");
        return NULL;
    }
    
    // Initialisiere Message Queue
    key_t key = ftok(".", 'P');
    handler->msg_queue_id = msgget(key, IPC_CREAT | 0666);
    if (handler->msg_queue_id == -1) {
        free(handler);
        perror("Failed to create message queue");
        return NULL;
    }
    
    // Öffne Log-Datei
    handler->log_file = fopen(log_file_path, "a");
    if (!handler->log_file) {
        msgctl(handler->msg_queue_id, IPC_RMID, NULL);
        free(handler);
        perror("Failed to open log file");
        return NULL;
    }
    
    handler->allocated_resources = 0;
    log_event(handler, "Process handler initialized successfully", "INFO");
    return handler;
}

// Validiere eingehende Nachricht
int validate_message(const Message* msg) {
    if (msg->magic != MAGIC_NUMBER) {
        return -1;  // Ungültige Magic Number
    }
    
    if (msg->msg_size > MAX_MSG_SIZE) {
        return -2;  // Nachricht zu groß
    }
    
    unsigned int calculated_checksum = calculate_checksum(msg);
    if (calculated_checksum != msg->checksum) {
        return -3;  // Prüfsumme ungültig
    }
    
    return 0;
}

// Sende Nachricht
int send_message(ProcessHandler* handler, Message* msg) {
    char log_buffer[MAX_LOG_SIZE];
    
    // Setze Magic Number und berechne Prüfsumme
    msg->magic = MAGIC_NUMBER;
    msg->checksum = calculate_checksum(msg);
    
    // Sende Nachricht
    if (msgsnd(handler->msg_queue_id, msg, sizeof(Message) - sizeof(long), 0) == -1) {
        snprintf(log_buffer, MAX_LOG_SIZE, "Failed to send message: %s", strerror(errno));
        log_event(handler, log_buffer, "ERROR");
        return -1;
    }
    
    handler->allocated_resources++;
    snprintf(log_buffer, MAX_LOG_SIZE, "Message sent successfully, type: %d", msg->msg_type);
    log_event(handler, log_buffer, "INFO");
    return 0;
}

// Empfange Nachricht
int receive_message(ProcessHandler* handler, Message* msg, int msg_type) {
    char log_buffer[MAX_LOG_SIZE];
    
    // Empfange Nachricht
    if (msgrcv(handler->msg_queue_id, msg, sizeof(Message) - sizeof(long), msg_type, 0) == -1) {
        snprintf(log_buffer, MAX_LOG_SIZE, "Failed to receive message: %s", strerror(errno));
        log_event(handler, log_buffer, "ERROR");
        return -1;
    }
    
    // Validiere Nachricht
    int validation_result = validate_message(msg);
    if (validation_result != 0) {
        snprintf(log_buffer, MAX_LOG_SIZE, "Message validation failed: %d", validation_result);
        log_event(handler, log_buffer, "ERROR");
        return validation_result;
    }
    
    handler->allocated_resources--;
    log_event(handler, "Message received and validated successfully", "INFO");
    return 0;
}

// Cleanup und Ressourcen freigeben
void cleanup_process_handler(ProcessHandler* handler) {
    if (!handler) return;
    
    // Schließe Message Queue
    if (handler->msg_queue_id != -1) {
        msgctl(handler->msg_queue_id, IPC_RMID, NULL);
    }
    
    // Schließe Log-Datei
    if (handler->log_file) {
        log_event(handler, "Process handler cleanup initiated", "INFO");
        fclose(handler->log_file);
    }
    
    // Überprüfe auf Memory Leaks
    if (handler->allocated_resources != 0) {
        fprintf(stderr, "Warning: %d resources still allocated during cleanup\n",
                handler->allocated_resources);
    }
    
    free(handler);
}

// Beispiel für die Verwendung
int main() {
    // Initialisiere Handler
    ProcessHandler* handler = init_process_handler("process_handler.log");
    if (!handler) {
        fprintf(stderr, "Failed to initialize process handler\n");
        return 1;
    }
    
    // Beispielnachricht erstellen und senden
    Message msg = {0};
    msg.msg_type = 1;
    msg.msg_size = snprintf(msg.payload, MAX_MSG_SIZE, "Test message");
    
    if (send_message(handler, &msg) != 0) {
        fprintf(stderr, "Failed to send message\n");
        cleanup_process_handler(handler);
        return 1;
    }
    
    // Nachricht empfangen
    Message received_msg = {0};
    if (receive_message(handler, &received_msg, 1) != 0) {
        fprintf(stderr, "Failed to receive message\n");
        cleanup_process_handler(handler);
        return 1;
    }
    
    // Cleanup
    cleanup_process_handler(handler);
    return 0;
}