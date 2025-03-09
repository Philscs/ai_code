#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

// Funktion zum Überprüfen, ob ein Pfad gültig ist
bool is_valid_path(const char *path) {
    return access(path, F_OK) == 0;
}

// Funktion zum Prüfen der Sicherheit von Symbolischen Link
void check_symbolic_link(const struct stat *st, const char *link_name, const char *target_name) {
    // Überprüfe, ob das Ziel des Linkes existiert
    if (access(target_name, F_OK) != 0) {
        fprintf(stderr, "Error: Ziel '%s' von Symbolischem Link '%s' nicht gefunden\n", 
target_name, link_name);
        return;
    }
}

// Funktion zum Überprüfen der Berechtigungen eines Benutzers
bool check_permissions(const struct stat *st, const char *user) {
    // Wenn der Benutzer die ausführende Datei besitzt oder über die notwendigen Rechte verfügt
    if (strcmp(user, st->st_uid) == 0 || strcmp(st->st_gid, st->st_gid) == 0) {
        return true;
    }
    return false;
}

// Funktion zum Sanitieren eines Pfads
char *sanitize_path(const char *path) {
    size_t path_len = strlen(path);
    char *sanitized_path = malloc((path_len + 1) * sizeof(char));
    if (sanitized_path == NULL) {
        fprintf(stderr, "Error: Nicht genügend Speicher für Pfad '%s'\n", path);
        return NULL;
    }
    strncpy(sanitized_path, path, path_len);
    sanitized_path[path_len] = '\0';
    return sanitized_path;
}

// Funktion zum Lesen der Datei-Informationen
int get_file_info(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        fprintf(stderr, "Error: Konnte Datei '%s' nicht lesen\n", path);
        return 1;
    }
    return 0;
}

// Funktion zum Iterieren über alle Dateien und Ordner im File-System
void iterate_over_files(const char *root_path) {
    DIR *dir = opendir(root_path);
    if (dir == NULL) {
        fprintf(stderr, "Error: Konnte Directory '%s' nicht öffnen\n", root_path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *entry_name = entry->d_name;

        // Überprüfe, ob der Pfad gültig ist
        if (!is_valid_path(entry_name)) {
            continue;
        }

        // Sanitieren des Pfades
        const char *sanitized_entry_name = sanitize_path(entry_name);
        if (sanitized_entry_name == NULL) {
            continue;
        }

        // Überprüfe, ob der Pfad ein Symbolischer Link ist
        struct stat st;
        if (stat(sanitized_entry_name, &st) == 0 && S_ISLNK(st.st_mode)) {
            check_symbolic_link(&st, sanitized_entry_name, st.st_pathname);
            continue;
        }

        // Überprüfe die Berechtigungen des Benutzers
        const char *user = getenv("USER");
        if (check_permissions(&st, user) == false) {
            continue;
        }

        // Lesen der Datei-Informationen
        int status = get_file_info(sanitized_entry_name);
        if (status != 0) {
            continue;
        }

        printf("%s\n", sanitized_entry_name);
    }
    closedir(dir);
}

int main() {
    iterate_over_files(".");
    return 0;
}