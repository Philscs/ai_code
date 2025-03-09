#include <stdio.h>
#include <stdlib.h>

// Define a custom memory allocation function
void* custom_malloc(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    printf("Allocated %zu bytes at %p in %s:%d\n", size, ptr, file, line);
    return ptr;
}

// Define a custom memory deallocation function
void custom_free(void* ptr, const char* file, int line) {
    free(ptr);
    printf("Freed memory at %p in %s:%d\n", ptr, file, line);
}

// Override the default malloc and free functions
#define malloc(size) custom_malloc(size, __FILE__, __LINE__)
#define free(ptr) custom_free(ptr, __FILE__, __LINE__)

// Usage example
int main() {
    int* ptr = malloc(sizeof(int));
    *ptr = 10;
    free(ptr);
    return 0;
}
