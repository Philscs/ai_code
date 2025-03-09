#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>
#include <string.h>

#define STACK_DEPTH 10
#define LOG_FILE "signal_handler.log"

// Function to log messages in a file
void log_message(const char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fputs(message, log_file);
        fputs("\n", log_file);
        fclose(log_file);
    }
}

// Signal-safe logging (simple implementation)
void safe_log_message(const char *message) {
    write(STDERR_FILENO, message, strlen(message));
    write(STDERR_FILENO, "\n", 1);
}

// Function to handle stack traces
void print_stack_trace() {
    void *buffer[STACK_DEPTH];
    int frames = backtrace(buffer, STACK_DEPTH);
    backtrace_symbols_fd(buffer, frames, STDERR_FILENO);
}

// Custom signal handler
void signal_handler(int signo, siginfo_t *info, void *context) {
    // Log the signal
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Received signal %d (address: %p)", signo, info->si_addr);
    safe_log_message(buffer);

    // Print stack trace
    safe_log_message("Stack trace:");
    print_stack_trace();

    // Stack Overflow Protection: For SIGSEGV, exit immediately
    if (signo == SIGSEGV) {
        safe_log_message("Stack overflow or segmentation fault detected. Exiting...");
        _exit(EXIT_FAILURE);
    }

    // Perform resource cleanup
    log_message("Performing resource cleanup...");
    
    // Attempt state recovery if possible
    if (signo == SIGUSR1) {
        log_message("Recovering from signal...");
        return; // Example recovery logic
    }

    // Exit gracefully
    log_message("Terminating application.");
    _exit(EXIT_FAILURE);
}

// Function to set up the custom signal handler
void setup_signal_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO; // Use sa_sigaction
    sa.sa_sigaction = signal_handler;

    // Register signals
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("Failed to set handler for SIGSEGV");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Failed to set handler for SIGUSR1");
        exit(EXIT_FAILURE);
    }
}

int main() {
    log_message("Application started.");

    // Setup custom signal handler
    setup_signal_handler();

    // Simulate a signal (for testing purposes)
    raise(SIGUSR1);

    // Cause a segmentation fault to test the handler
    // Uncomment the following line to trigger SIGSEGV:
    // int *p = NULL; *p = 42;

    log_message("Application finished normally.");
    return 0;
}
