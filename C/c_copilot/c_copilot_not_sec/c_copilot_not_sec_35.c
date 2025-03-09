#include <stdio.h>
#include <stdlib.h>
#include <numa.h>

#define PAGE_SIZE (4096) // Change this to the desired page size

void* custom_malloc(size_t size) {
    void* ptr = NULL;
    int numa_node = -1;

    // Determine the NUMA node for allocation
    if (numa_available() >= 0) {
        numa_node = numa_node_of_cpu(sched_getcpu());
    }

    // Allocate memory using large pages if size is larger than page size
    if (size > PAGE_SIZE) {
        if (numa_node >= 0) {
            ptr = numa_alloc_onnode(size, numa_node);
        } else {
            ptr = malloc(size);
        }
    } else {
        ptr = malloc(size);
    }

    return ptr;
}

void custom_free(void* ptr) {
    if (numa_available() >= 0) {
        numa_free(ptr, PAGE_SIZE);
    } else {
        free(ptr);
    }
}

int main() {
    // Usage example
    int* arr = (int*)custom_malloc(100 * sizeof(int));
    if (arr != NULL) {
        // Use the allocated memory
        for (int i = 0; i < 100; i++) {
            arr[i] = i;
        }

        // Free the memory
        custom_free(arr);
    }

    return 0;
}
