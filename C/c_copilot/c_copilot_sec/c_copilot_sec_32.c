#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define FILEPATH "/path/to/file"
#define FILESIZE 4096

int main() {
    int fd;
    void *mapped_data;

    // Open the file
    fd = open(FILEPATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Set the file size
    if (ftruncate(fd, FILESIZE) == -1) {
        perror("Error setting file size");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Map the file into memory
    mapped_data = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_data == MAP_FAILED) {
        perror("Error mapping file into memory");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Perform operations on the mapped data
    strcpy(mapped_data, "Hello, World!");

    // Unmap the file from memory
    if (munmap(mapped_data, FILESIZE) == -1) {
        perror("Error unmapping file from memory");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Close the file
    if (close(fd) == -1) {
        perror("Error closing file");
        exit(EXIT_FAILURE);
    }

    return 0;
}
