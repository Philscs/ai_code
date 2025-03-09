#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>

// Constants for stack overflow protection
#define ALTERNATE_STACK_SIZE (SIGSTKSZ * 2)
#define LOG_BUFFER_SIZE 1024

// Global variables for state recovery
static volatile sig_atomic_t signal_received = 0;
static volatile sig_atomic_t cleanup_in_progress = 0;

// Pre-allocated buffer for async-safe logging
static char log_buffer[LOG_BUFFER_SIZE];
static int log_fd = -1;

// Resource tracking structure
typedef struct {
    void *ptr;
    void (*cleanup_func)(void*);
} Resource;

#define MAX_RESOURCES 32
static Resource resources[MAX_RESOURCES];
static volatile sig_atomic_t resource_count = 0;

// Async-signal-safe logging function
static void safe_write_log(const char *msg) {
    if (log_fd != -1) {
        size_t len = strlen(msg);
        ssize_t ret;
        do {
            ret = write(log_fd, msg, len);
        } while (ret == -1 && errno == EINTR);
    }
}

// Initialize alternate stack for stack overflow protection
static int setup_alternate_stack(void) {
    stack_t ss;
    ss.ss_sp = mmap(NULL, ALTERNATE_STACK_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (ss.ss_sp == MAP_FAILED) {
        return -1;
    }

    ss.ss_size = ALTERNATE_STACK_SIZE;
    ss.ss_flags = 0;

    if (sigaltstack(&ss, NULL) == -1) {
        munmap(ss.ss_sp, ALTERNATE_STACK_SIZE);
        return -1;
    }

    return 0;
}

// Resource management functions
static int register_resource(void *ptr, void (*cleanup_func)(void*)) {
    if (resource_count >= MAX_RESOURCES) {
        return -1;
    }

    resources[resource_count].ptr = ptr;
    resources[resource_count].cleanup_func = cleanup_func;
    resource_count++;
    return 0;
}

static void cleanup_resources(void) {
    if (cleanup_in_progress) {
        return;  // Prevent recursive cleanup
    }
    cleanup_in_progress = 1;

    // Cleanup resources in reverse order
    while (resource_count > 0) {
        resource_count--;
        if (resources[resource_count].cleanup_func && resources[resource_count].ptr) {
            resources[resource_count].cleanup_func(resources[resource_count].ptr);
        }
    }

    cleanup_in_progress = 0;
}

// Custom signal handler
static void signal_handler(int signum, siginfo_t *info, void *context) {
    // Record signal for state recovery
    signal_received = signum;

    // Async-safe logging
    time_t now = time(NULL);
    snprintf(log_buffer, LOG_BUFFER_SIZE,
             "Signal %d received at %ld. Sender PID: %ld\n",
             signum, (long)now, (long)info->si_pid);
    safe_write_log(log_buffer);

    // Perform cleanup
    cleanup_resources();

    // State recovery actions based on signal type
    switch (signum) {
        case SIGSEGV:
        case SIGFPE:
        case SIGILL:
            safe_write_log("Fatal signal received. Performing emergency shutdown.\n");
            _exit(EXIT_FAILURE);
            break;
        
        case SIGTERM:
        case SIGINT:
            safe_write_log("Termination signal received. Performing graceful shutdown.\n");
            exit(EXIT_SUCCESS);
            break;

        default:
            // For other signals, we might want to restore state and continue
            safe_write_log("Recoverable signal received. Attempting to continue.\n");
            break;
    }
}

// Initialize the signal handling system
int init_signal_handler(const char *log_path) {
    // Open log file
    log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        return -1;
    }

    // Set up alternate stack for handling stack overflow
    if (setup_alternate_stack() != 0) {
        close(log_fd);
        return -1;
    }

    // Set up signal handling
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;

    // Block all signals during signal handler execution
    sigfillset(&sa.sa_mask);

    // Register handler for various signals
    const int signals[] = {
        SIGSEGV, SIGFPE, SIGILL, SIGTERM, SIGINT, SIGABRT
    };
    
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
        if (sigaction(signals[i], &sa, NULL) == -1) {
            close(log_fd);
            return -1;
        }
    }

    return 0;
}

// Example usage and cleanup function
void cleanup_file(void *ptr) {
    if (ptr) {
        fclose((FILE *)ptr);
    }
}

// Example main function showing usage
int main(void) {
    // Initialize signal handler with log file
    if (init_signal_handler("/var/log/app.log") == -1) {
        fprintf(stderr, "Failed to initialize signal handler\n");
        return 1;
    }

    // Example resource registration
    FILE *fp = fopen("example.txt", "w");
    if (fp) {
        register_resource(fp, cleanup_file);
    }

    // Main program loop
    while (!signal_received) {
        // Normal program operation
        sleep(1);
    }

    return 0;
}