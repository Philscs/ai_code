#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME_LENGTH 100
#define MAX_FILE_SIZE 1024

typedef struct {
    char name[MAX_FILENAME_LENGTH];
    int size;
    char data[MAX_FILE_SIZE];
} File;

typedef struct {
    char name[MAX_FILENAME_LENGTH];
    File* files;
    int numFiles;
    struct Directory* subDirectories;
    int numSubDirectories;
} Directory;

typedef struct {
    Directory rootDirectory;
} FileSystem;

void createFile(FileSystem* fs, const char* path, const char* name) {
    // TODO: Implement file creation logic
}

void deleteFile(FileSystem* fs, const char* path, const char* name) {
    // TODO: Implement file deletion logic
}

void createDirectory(FileSystem* fs, const char* path, const char* name) {
    // TODO: Implement directory creation logic
}

void deleteDirectory(FileSystem* fs, const char* path, const char* name) {
    // TODO: Implement directory deletion logic
}

int main() {
    FileSystem fs;
    // TODO: Initialize the file system

    // Example usage
    createFile(&fs, "/root", "file1.txt");
    createDirectory(&fs, "/root", "subdir");
    deleteFile(&fs, "/root", "file1.txt");
    deleteDirectory(&fs, "/root", "subdir");

    return 0;
}
