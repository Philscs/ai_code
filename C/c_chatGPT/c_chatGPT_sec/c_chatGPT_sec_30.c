#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_KEY_LENGTH 100
#define MAX_VALUE_LENGTH 200
#define CONFIG_FILE "config.cfg"
#define LOG_FILE "generator.log"

void log_message(const char *message) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Error opening log file: %s\n", strerror(errno));
        return;
    }
    fprintf(log_file, "%s\n", message);
    fclose(log_file);
}

bool validate_input(const char *key, const char *value) {
    if (strlen(key) == 0 || strlen(value) == 0) {
        log_message("Validation failed: Key or value is empty.");
        return false;
    }

    if (strlen(key) > MAX_KEY_LENGTH || strlen(value) > MAX_VALUE_LENGTH) {
        log_message("Validation failed: Key or value exceeds maximum length.");
        return false;
    }

    for (size_t i = 0; i < strlen(key); i++) {
        if (key[i] == ' ' || key[i] == '=') {
            log_message("Validation failed: Key contains invalid characters.");
            return false;
        }
    }

    return true;
}

bool write_to_config_file(const char *key, const char *value) {
    FILE *config_file = fopen(CONFIG_FILE, "a");
    if (config_file == NULL) {
        char error_message[256];
        snprintf(error_message, sizeof(error_message), "Error opening config file: %s", strerror(errno));
        log_message(error_message);
        return false;
    }

    if (fprintf(config_file, "%s=%s\n", key, value) < 0) {
        char error_message[256];
        snprintf(error_message, sizeof(error_message), "Error writing to config file: %s", strerror(errno));
        log_message(error_message);
        fclose(config_file);
        return false;
    }

    fclose(config_file);
    return true;
}

void generate_config() {
    char key[MAX_KEY_LENGTH + 1];
    char value[MAX_VALUE_LENGTH + 1];

    printf("Enter key: ");
    if (fgets(key, sizeof(key), stdin) == NULL) {
        log_message("Error reading key input.");
        return;
    }
    key[strcspn(key, "\n")] = '\0'; // Remove newline character

    printf("Enter value: ");
    if (fgets(value, sizeof(value), stdin) == NULL) {
        log_message("Error reading value input.");
        return;
    }
    value[strcspn(value, "\n")] = '\0'; // Remove newline character

    if (!validate_input(key, value)) {
        printf("Invalid input. Please check the log for details.\n");
        return;
    }

    if (write_to_config_file(key, value)) {
        printf("Config entry added successfully.\n");
        log_message("Config entry added successfully.");
    } else {
        printf("Failed to write to config file. Please check the log for details.\n");
    }
}

int main() {
    printf("Secure Config File Generator\n");
    printf("============================\n");

    char choice;
    do {
        generate_config();
        printf("Do you want to add another entry? (y/n): ");
        scanf(" %c", &choice);
        getchar(); // Consume newline character left by scanf
    } while (choice == 'y' || choice == 'Y');

    printf("Exiting.\n");
    return 0;
}
