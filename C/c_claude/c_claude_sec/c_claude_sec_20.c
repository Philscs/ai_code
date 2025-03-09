#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>

#define MAX_PATH_LEN PATH_MAX
#define ERROR_BUFFER_SIZE 256

typedef struct {
    char error_msg[ERROR_BUFFER_SIZE];
    int error_code;
} ErrorInfo;

// Funktion zur Pfadbereinigung
char* sanitize_path(const char* input_path, ErrorInfo* error) {
    char* clean_path = realpath(input_path, NULL);
    if (clean_path == NULL) {
        snprintf(error->error_msg, ERROR_BUFFER_SIZE, 
                "Fehler bei Pfadbereinigung: %s", strerror(errno));
        error->error_code = errno;
        return NULL;
    }
    return clean_path;
}

// Überprüfung der Berechtigungen
int check_permissions(const char* path, ErrorInfo* error) {
    struct stat st;
    
    if (lstat(path, &st) != 0) {
        snprintf(error->error_msg, ERROR_BUFFER_SIZE,
                "Fehler beim Lesen der Dateiattribute: %s", strerror(errno));
        error->error_code = errno;
        return -1;
    }

    // Überprüfe ob der aktuelle Benutzer Leserechte hat
    uid_t uid = getuid();
    if (st.st_uid == uid) {
        if (!(st.st_mode & S_IRUSR)) {
            snprintf(error->error_msg, ERROR_BUFFER_SIZE,
                    "Keine Leseberechtigung für den Benutzer");
            error->error_code = EACCES;
            return -1;
        }
    } else {
        // Überprüfe Gruppenrechte
        if (!(st.st_mode & S_IRGRP)) {
            snprintf(error->error_msg, ERROR_BUFFER_SIZE,
                    "Keine Leseberechtigung für die Gruppe");
            error->error_code = EACCES;
            return -1;
        }
    }

    return 0;
}

// Behandlung von symbolischen Links
int handle_symlink(const char* path, ErrorInfo* error) {
    struct stat st;
    
    if (lstat(path, &st) != 0) {
        snprintf(error->error_msg, ERROR_BUFFER_SIZE,
                "Fehler beim Lesen des symbolischen Links: %s", strerror(errno));
        error->error_code = errno;
        return -1;
    }

    if (S_ISLNK(st.st_mode)) {
        char link_target[MAX_PATH_LEN];
        ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
        
        if (len == -1) {
            snprintf(error->error_msg, ERROR_BUFFER_SIZE,
                    "Fehler beim Folgen des symbolischen Links: %s", strerror(errno));
            error->error_code = errno;
            return -1;
        }
        
        link_target[len] = '\0';
        
        // Überprüfe ob das Ziel außerhalb des erlaubten Bereichs liegt
        char* resolved_target = realpath(link_target, NULL);
        if (resolved_target == NULL) {
            snprintf(error->error_msg, ERROR_BUFFER_SIZE,
                    "Ungültiges Link-Ziel");
            error->error_code = EACCES;
            return -1;
        }
        
        free(resolved_target);
    }
    
    return 0;
}

// Hauptfunktion zum Durchlaufen des Dateisystems
int walk_directory(const char* start_path, void (*callback)(const char*, void*), 
                  void* user_data, ErrorInfo* error) {
    DIR* dir;
    struct dirent* entry;
    char* clean_path = NULL;
    int result = 0;

    // Pfadbereinigung
    clean_path = sanitize_path(start_path, error);
    if (clean_path == NULL) {
        return -1;
    }

    // Berechtigungsprüfung
    if (check_permissions(clean_path, error) != 0) {
        free(clean_path);
        return -1;
    }

    // Öffne das Verzeichnis
    dir = opendir(clean_path);
    if (dir == NULL) {
        snprintf(error->error_msg, ERROR_BUFFER_SIZE,
                "Fehler beim Öffnen des Verzeichnisses: %s", strerror(errno));
        error->error_code = errno;
        free(clean_path);
        return -1;
    }

    // Durchlaufe alle Einträge
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", clean_path, entry->d_name);

        // Behandle symbolische Links
        if (handle_symlink(full_path, error) != 0) {
            result = -1;
            break;
        }

        // Rufe Callback-Funktion auf
        callback(full_path, user_data);

        // Wenn es sich um ein Verzeichnis handelt, rekursiv durchlaufen
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            if (walk_directory(full_path, callback, user_data, error) != 0) {
                result = -1;
                break;
            }
        }
    }

    // Aufräumen
    closedir(dir);
    free(clean_path);

    return result;
}

// Beispiel-Callback-Funktion
void print_file(const char* path, void* user_data) {
    printf("Gefundene Datei: %s\n", path);
}

// Beispiel für die Verwendung
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Verwendung: %s <verzeichnis>\n", argv[0]);
        return 1;
    }

    ErrorInfo error = {0};
    
    if (walk_directory(argv[1], print_file, NULL, &error) != 0) {
        fprintf(stderr, "Fehler: %s (Code: %d)\n", 
                error.error_msg, error.error_code);
        return 1;
    }

    return 0;
}