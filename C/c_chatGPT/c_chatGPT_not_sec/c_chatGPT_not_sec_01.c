#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Struktur für einen Prozess
typedef struct {
    int pid;              // Prozess-ID
    int priority;         // Priorität (niedrigere Zahl = höhere Priorität)
    int cpu_burst_time;   // CPU-Burst-Zeit
    int waiting_time;     // Wartezeit
} Process;

// Struktur für den Min-Heap
typedef struct {
    Process* heap;
    int size;
    int capacity;
} MinHeap;

// Hilfsfunktionen für den Min-Heap
MinHeap* createMinHeap(int capacity) {
    MinHeap* minHeap = (MinHeap*)malloc(sizeof(MinHeap));
    minHeap->heap = (Process*)malloc(sizeof(Process) * capacity);
    minHeap->size = 0;
    minHeap->capacity = capacity;
    return minHeap;
}

void swap(Process* a, Process* b) {
    Process temp = *a;
    *a = *b;
    *b = temp;
}

void heapifyUp(MinHeap* minHeap, int index) {
    int parent = (index - 1) / 2;
    if (index > 0 && minHeap->heap[index].priority < minHeap->heap[parent].priority) {
        swap(&minHeap->heap[index], &minHeap->heap[parent]);
        heapifyUp(minHeap, parent);
    }
}

void heapifyDown(MinHeap* minHeap, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < minHeap->size && minHeap->heap[left].priority < minHeap->heap[smallest].priority) {
        smallest = left;
    }

    if (right < minHeap->size && minHeap->heap[right].priority < minHeap->heap[smallest].priority) {
        smallest = right;
    }

    if (smallest != index) {
        swap(&minHeap->heap[index], &minHeap->heap[smallest]);
        heapifyDown(minHeap, smallest);
    }
}

void insertProcess(MinHeap* minHeap, Process process) {
    if (minHeap->size == minHeap->capacity) {
        printf("Heap ist voll. Kann Prozess nicht hinzufügen.\n");
        return;
    }
    minHeap->heap[minHeap->size] = process;
    minHeap->size++;
    heapifyUp(minHeap, minHeap->size - 1);
}

Process extractMinProcess(MinHeap* minHeap) {
    if (minHeap->size == 0) {
        printf("Heap ist leer.\n");
        exit(EXIT_FAILURE);
    }
    Process minProcess = minHeap->heap[0];
    minHeap->heap[0] = minHeap->heap[minHeap->size - 1];
    minHeap->size--;
    heapifyDown(minHeap, 0);
    return minProcess;
}

void calculateWaitingTimes(MinHeap* minHeap) {
    int currentTime = 0;
    for (int i = 0; i < minHeap->size; i++) {
        minHeap->heap[i].waiting_time = currentTime;
        currentTime += minHeap->heap[i].cpu_burst_time;
    }
}

void printProcesses(MinHeap* minHeap) {
    printf("PID\tPriorität\tCPU-Burst\tWartezeit\n");
    for (int i = 0; i < minHeap->size; i++) {
        printf("%d\t%d\t\t%d\t\t%d\n",
               minHeap->heap[i].pid,
               minHeap->heap[i].priority,
               minHeap->heap[i].cpu_burst_time,
               minHeap->heap[i].waiting_time);
    }
}

int main() {
    int capacity = 10;
    MinHeap* scheduler = createMinHeap(capacity);

    // Prozesse einfügen
    insertProcess(scheduler, (Process){1, 2, 5, 0});
    insertProcess(scheduler, (Process){2, 1, 3, 0});
    insertProcess(scheduler, (Process){3, 3, 2, 0});
    insertProcess(scheduler, (Process){4, 0, 1, 0});

    printf("Prozesse nach Priorität sortiert:\n");
    calculateWaitingTimes(scheduler);
    printProcesses(scheduler);

    printf("\nProzesse werden ausgeführt:\n");
    while (scheduler->size > 0) {
        Process p = extractMinProcess(scheduler);
        printf("Prozess %d mit Priorität %d und CPU-Burst-Zeit %d wird ausgeführt.\n",
               p.pid, p.priority, p.cpu_burst_time);
    }

    free(scheduler->heap);
    free(scheduler);
    return 0;
}