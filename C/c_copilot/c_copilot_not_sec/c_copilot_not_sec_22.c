#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE 4096
#define NUM_PAGES 256
#define PHYSICAL_MEMORY_SIZE (PAGE_SIZE * NUM_PAGES)

typedef struct {
    int valid;
    int frame_number;
} PageTableEntry;

typedef struct {
    int page_number;
    int accessed;
} PageTableEntryWithAccess;

typedef struct {
    int frame_number;
    int referenced;
} PhysicalMemoryEntry;

PageTableEntry page_table[NUM_PAGES];
PhysicalMemoryEntry physical_memory[NUM_PAGES];

int page_faults = 0;

void initialize_page_table() {
    for (int i = 0; i < NUM_PAGES; i++) {
        page_table[i].valid = 0;
        page_table[i].frame_number = -1;
    }
}

void initialize_physical_memory() {
    for (int i = 0; i < NUM_PAGES; i++) {
        physical_memory[i].frame_number = -1;
        physical_memory[i].referenced = 0;
    }
}

int find_free_frame() {
    for (int i = 0; i < NUM_PAGES; i++) {
        if (physical_memory[i].frame_number == -1) {
            return i;
        }
    }
    return -1; // No free frame available
}

int find_page_to_replace() {
    int min_referenced = physical_memory[0].referenced;
    int min_referenced_index = 0;

    for (int i = 1; i < NUM_PAGES; i++) {
        if (physical_memory[i].referenced < min_referenced) {
            min_referenced = physical_memory[i].referenced;
            min_referenced_index = i;
        }
    }

    return min_referenced_index;
}

void handle_page_fault(int page_number) {
    page_faults++;

    int free_frame = find_free_frame();
    if (free_frame != -1) {
        page_table[page_number].valid = 1;
        page_table[page_number].frame_number = free_frame;
        physical_memory[free_frame].frame_number = page_number;
        physical_memory[free_frame].referenced = 1;
    } else {
        int page_to_replace = find_page_to_replace();
        int frame_to_replace = page_table[page_to_replace].frame_number;

        page_table[page_to_replace].valid = 0;
        page_table[page_number].valid = 1;
        page_table[page_number].frame_number = frame_to_replace;
        physical_memory[frame_to_replace].frame_number = page_number;
        physical_memory[frame_to_replace].referenced = 1;
    }
}

void access_memory(int virtual_address) {
    int page_number = virtual_address / PAGE_SIZE;

    if (page_table[page_number].valid) {
        int frame_number = page_table[page_number].frame_number;
        physical_memory[frame_number].referenced = 1;
        printf("Accessing physical address: %d\n", frame_number * PAGE_SIZE + (virtual_address % PAGE_SIZE));
    } else {
        handle_page_fault(page_number);
        printf("Page fault occurred. Accessing physical address: %d\n", page_table[page_number].frame_number * PAGE_SIZE + (virtual_address % PAGE_SIZE));
    }
}

int main() {
    initialize_page_table();
    initialize_physical_memory();

    access_memory(8192);
    access_memory(4096);
    access_memory(12288);
    access_memory(8192);
    access_memory(16384);

    printf("Total page faults: %d\n", page_faults);

    return 0;
}
