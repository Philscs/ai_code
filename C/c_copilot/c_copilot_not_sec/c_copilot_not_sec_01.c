#include <stdio.h>
#include <stdlib.h>

// Process structure
typedef struct {
    int priority;
    int cpuBurstTime;
    int waitTime;
} Process;

// Heap structure
typedef struct {
    Process* processes;
    int size;
    int capacity;
} Heap;

// Function to swap two processes
void swap(Process* a, Process* b) {
    Process temp = *a;
    *a = *b;
    *b = temp;
}

// Function to initialize the heap
Heap* initHeap(int capacity) {
    Heap* heap = (Heap*)malloc(sizeof(Heap));
    heap->processes = (Process*)malloc(capacity * sizeof(Process));
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

// Function to insert a process into the heap
void insertProcess(Heap* heap, Process process) {
    if (heap->size == heap->capacity) {
        printf("Heap is full. Cannot insert process.\n");
        return;
    }

    // Insert the process at the end
    heap->processes[heap->size] = process;
    int i = heap->size;
    heap->size++;

    // Heapify up
    while (i > 0 && heap->processes[i].priority < heap->processes[(i - 1) / 2].priority) {
        swap(&heap->processes[i], &heap->processes[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

// Function to extract the process with highest priority from the heap
Process extractMaxProcess(Heap* heap) {
    if (heap->size == 0) {
        printf("Heap is empty. Cannot extract process.\n");
        Process emptyProcess = {0, 0, 0};
        return emptyProcess;
    }

    // Extract the root process
    Process maxProcess = heap->processes[0];

    // Replace the root with the last process
    heap->processes[0] = heap->processes[heap->size - 1];
    heap->size--;

    // Heapify down
    int i = 0;
    while (1) {
        int leftChild = 2 * i + 1;
        int rightChild = 2 * i + 2;
        int smallest = i;

        if (leftChild < heap->size && heap->processes[leftChild].priority < heap->processes[smallest].priority) {
            smallest = leftChild;
        }

        if (rightChild < heap->size && heap->processes[rightChild].priority < heap->processes[smallest].priority) {
            smallest = rightChild;
        }

        if (smallest != i) {
            swap(&heap->processes[i], &heap->processes[smallest]);
            i = smallest;
        } else {
            break;
        }
    }

    return maxProcess;
}

int main() {
    // Create a heap with capacity 10
    Heap* heap = initHeap(10);

    // Insert some processes into the heap
    Process p1 = {3, 10, 0};
    Process p2 = {1, 5, 0};
    Process p3 = {2, 8, 0};
    insertProcess(heap, p1);
    insertProcess(heap, p2);
    insertProcess(heap, p3);

    // Extract the processes with highest priority
    Process maxProcess = extractMaxProcess(heap);
    printf("Process with highest priority: Priority=%d, CPU Burst Time=%d, Wait Time=%d\n", maxProcess.priority, maxProcess.cpuBurstTime, maxProcess.waitTime);

    // Free the heap memory
    free(heap->processes);
    free(heap);

    return 0;
}
