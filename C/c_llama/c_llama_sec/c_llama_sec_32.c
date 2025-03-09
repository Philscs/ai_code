#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_FILE_SIZE 1024 * 1024 * 1024 // max file size in bytes
#define MAX_MEMORY_ADDRESS 0x7fffffff00 // max memory address in hex

typedef struct {
    int fd;
    void* mem_addr;
    size_t mem_size;
} mmap_file_t;

typedef enum {
    PERMISSION_READ,
    PERMISSION_WRITE,
    PERMISSION_EXECUTE
} permission_t;

typedef enum {
    ERROR_OK = 0,
    ERROR_PERMISSION_DENIED = -1,
    ERROR_MEMORY_ALLOCATION_FAILED = -2,
    ERROR_MAP_FAILED = -3
} error_code_t;

// Funktion zum Erstellen eines Memory mapped Files
mmap_file_t* create_mmap_file(const char* filename, permission_t permissions) {
    mmap_file_t* mmap_file = malloc(sizeof(mmap_file_t));
    if (mmap_file == NULL) {
        return NULL;
    }

    // Öffnen des Dateisystems-Files
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        free(mmap_file);
        return NULL;
    }

    // Prüfen der Berechtigungen
    if (!(permissions & PERMISSION_READ)) {
        perror("Permission denied");
        close(fd);
        free(mmap_file);
        return NULL;
    }

    // Ersetzen der Dateisystem-File-Adresse durch eine Memory mapped Adresse
    void* mem_addr = mmap(NULL, MAX_FILE_SIZE, permissions, MS_PRIVATE | MS_ANONYMOUS | MS_WRITE, 
0);
    if (mem_addr == MAP_FAILED) {
        free(mmap_file);
        close(fd);
        return NULL;
    }

    // Speichern der Dateisystem-File-Adresse
    mmap_file->fd = fd;
    mmap_file->mem_addr = mem_addr;
    mmap_file->mem_size = MAX_FILE_SIZE;

    return mmap_file;
}

// Funktion zum Lesen aus einem Memory mapped File
void* mmap_read(mmap_file_t* mmap_file, size_t offset, size_t length) {
    if (mmap_file == NULL || mmap_file->fd == -1) {
        return NULL;
    }

    // Überprüfen der Berechtigungen
    if (!(offset & PERMISSION_READ)) {
        perror("Permission denied");
        return NULL;
    }

    // Lesen aus dem Dateisystems-File
    void* mem_addr = mmap(mmap_file->mem_addr, length, mmap_file->fd);
    if (mem_addr == MAP_FAILED) {
        return NULL;
    }

    return mem_addr;
}

// Funktion zum Schreiben in ein Memory mapped File
void mmap_write(mmap_file_t* mmap_file, size_t offset, const void* data, size_t length) {
    if (mmap_file == NULL || mmap_file->fd == -1) {
        return;
    }

    // Überprüfen der Berechtigungen
    if (!(offset & PERMISSION_WRITE)) {
        perror("Permission denied");
        return;
    }

    // Schreiben in das Dateisystems-File
    munmap(mmap_file->mem_addr, length);
    mmap_file->mem_addr = mmap(mmap_file->fd, length, mmap_file->fd);

    if (mmap_file->mem_addr == MAP_FAILED) {
        perror("Map failed");
        return;
    }

    memcpy(mmap_file->mem_addr + offset, data, length);
}

// Funktion zum Entfernen eines Memory mapped File
void destroy_mmap_file(mmap_file_t* mmap_file) {
    if (mmap_file != NULL && mmap_file->fd >= 0) {
        munmap(mmap_file->mem_addr, mmap_file->mem_size);

        close(mmap_file->fd);
        free(mmap_file);
    }
}

// Funktion zum Loggen eines Ereignisses
void log_event(const char* event_name, int status) {
    fprintf(stderr, "[%s] %d\n", event_name, status);
}
