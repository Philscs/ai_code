#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

void process_file(const char* path) {
    // Perform operations on the file
    printf("Processing file: %s\n", path);
}

void walk_directory(const char* directory) {
    DIR* dir = opendir(directory);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        struct stat st;
        if (lstat(path, &st) == -1) {
            perror("lstat");
            continue;
        }

        // Symbolic Link Safety
        if (S_ISLNK(st.st_mode)) {
            printf("Skipping symbolic link: %s\n", path);
            continue;
        }

        // Permission Checking
        if (!(st.st_mode & S_IRUSR)) {
            printf("Skipping file due to insufficient permissions: %s\n", path);
            continue;
        }

        // Path Sanitization
        if (strstr(path, "..") != NULL) {
            printf("Skipping file due to invalid path: %s\n", path);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            walk_directory(path);
        } else if (S_ISREG(st.st_mode)) {
            process_file(path);
        }
    }

    closedir(dir);
}

int main() {
    const char* directory = "/path/to/directory";
    walk_directory(directory);
    return 0;
}
