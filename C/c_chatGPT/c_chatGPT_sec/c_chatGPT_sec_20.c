#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>

void walk_directory(const char *path);
void handle_entry(const char *path, const struct stat *entry_stat);

void walk_directory(const char *path) {
    struct stat path_stat;
    if (lstat(path, &path_stat) == -1) {
        perror("lstat failed");
        return;
    }

    // Check if path is a symbolic link
    if (S_ISLNK(path_stat.st_mode)) {
        fprintf(stderr, "Skipping symbolic link: %s\n", path);
        return;
    }

    // Check if path is a directory
    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "Not a directory: %s\n", path);
        return;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir failed");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Build the full path
        char full_path[PATH_MAX];
        if (snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name) >= sizeof(full_path)) {
            fprintf(stderr, "Path too long: %s/%s\n", path, entry->d_name);
            continue;
        }

        struct stat entry_stat;
        if (lstat(full_path, &entry_stat) == -1) {
            perror("lstat failed");
            continue;
        }

        // Skip symbolic links
        if (S_ISLNK(entry_stat.st_mode)) {
            fprintf(stderr, "Skipping symbolic link: %s\n", full_path);
            continue;
        }

        // Handle the entry
        handle_entry(full_path, &entry_stat);

        // Recurse into directories
        if (S_ISDIR(entry_stat.st_mode)) {
            walk_directory(full_path);
        }
    }

    if (closedir(dir) == -1) {
        perror("closedir failed");
    }
}

void handle_entry(const char *path, const struct stat *entry_stat) {
    if (access(path, R_OK) != 0) {
        fprintf(stderr, "Permission denied: %s\n", path);
        return;
    }

    printf("Visited: %s\n", path);
    // Additional processing for files can be added here.
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    walk_directory(argv[1]);

    return EXIT_SUCCESS;
}
