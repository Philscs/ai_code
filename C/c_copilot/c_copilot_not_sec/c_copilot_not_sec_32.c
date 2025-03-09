#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>

typedef int (*JitFunction)();

int main() {
    // Define your subset of C code here
    
    // Allocate executable memory
    void* code = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (code == MAP_FAILED) {
        perror("Failed to allocate memory");
        exit(1);
    }
    
    // Generate machine code and write it to the allocated memory
    
    // Cast the memory as a function pointer
    JitFunction jitFunc = (JitFunction)code;
    
    // Call the JIT-compiled function
    int result = jitFunc();
    
    // Print the result
    printf("Result: %d\n", result);
    
    // Free the allocated memory
    munmap(code, 4096);
    
    return 0;
}
