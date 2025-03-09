#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Prozessstruktur
typedef struct {
    int id;                 // Prozess-ID
    int basePriority;      // Basis-Priorität (1-10)
    int dynamicPriority;   // Dynamische Priorität
    int burstTime;         // Benötigte CPU-Zeit
    int waitingTime;       // Wartezeit
    time_t arrivalTime;    // Ankunftszeit
} Process;

// Heap-Struktur
typedef struct {
    Process** processes;    // Array von Prozess-Zeigern
    int capacity;          // Maximale Kapazität
    int size;             // Aktuelle Größe
} PriorityQueue;

// Hilfsfunktionen für Heap-Operationen
void swap(Process** a, Process** b) {
    Process* temp = *a;
    *a = *b;
    *b = temp;
}

// Berechnet die dynamische Priorität eines Prozesses
void updateDynamicPriority(Process* process) {
    // Formel: Basis-Priorität + (Wartezeit / 10) - (Burst-Zeit / 5)
    process->dynamicPriority = process->basePriority + 
                              (process->waitingTime / 10) - 
                              (process->burstTime / 5);
    
    // Begrenze die dynamische Priorität auf einen Mindestwert von 1
    if (process->dynamicPriority < 1) {
        process->dynamicPriority = 1;
    }
}

// Initialisiert eine neue Priority Queue
PriorityQueue* createPriorityQueue(int capacity) {
    PriorityQueue* queue = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    queue->processes = (Process**)malloc(capacity * sizeof(Process*));
    queue->capacity = capacity;
    queue->size = 0;
    return queue;
}

// Heap-Operationen
void heapifyUp(PriorityQueue* queue, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (queue->processes[index]->dynamicPriority > 
            queue->processes[parent]->dynamicPriority) {
            swap(&queue->processes[index], &queue->processes[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

void heapifyDown(PriorityQueue* queue, int index) {
    int maxIndex = index;
    int leftChild = 2 * index + 1;
    int rightChild = 2 * index + 2;

    if (leftChild < queue->size && 
        queue->processes[leftChild]->dynamicPriority > 
        queue->processes[maxIndex]->dynamicPriority) {
        maxIndex = leftChild;
    }

    if (rightChild < queue->size && 
        queue->processes[rightChild]->dynamicPriority > 
        queue->processes[maxIndex]->dynamicPriority) {
        maxIndex = rightChild;
    }

    if (index != maxIndex) {
        swap(&queue->processes[index], &queue->processes[maxIndex]);
        heapifyDown(queue, maxIndex);
    }
}

// Fügt einen Prozess zur Queue hinzu
void enqueue(PriorityQueue* queue, Process* process) {
    if (queue->size >= queue->capacity) {
        printf("Queue ist voll!\n");
        return;
    }

    // Aktualisiere die dynamische Priorität vor dem Einfügen
    updateDynamicPriority(process);

    queue->processes[queue->size] = process;
    heapifyUp(queue, queue->size);
    queue->size++;
}

// Entfernt und gibt den Prozess mit höchster Priorität zurück
Process* dequeue(PriorityQueue* queue) {
    if (queue->size <= 0) {
        return NULL;
    }

    Process* highestPriorityProcess = queue->processes[0];
    queue->processes[0] = queue->processes[queue->size - 1];
    queue->size--;
    heapifyDown(queue, 0);

    return highestPriorityProcess;
}

// Aktualisiert die Wartezeiten aller Prozesse in der Queue
void updateWaitingTimes(PriorityQueue* queue) {
    for (int i = 0; i < queue->size; i++) {
        queue->processes[i]->waitingTime++;
        updateDynamicPriority(queue->processes[i]);
    }
    
    // Reorganisiere den Heap nach der Aktualisierung
    for (int i = queue->size / 2 - 1; i >= 0; i--) {
        heapifyDown(queue, i);
    }
}

// Erstellt einen neuen Prozess
Process* createProcess(int id, int priority, int burstTime) {
    Process* process = (Process*)malloc(sizeof(Process));
    process->id = id;
    process->basePriority = priority;
    process->burstTime = burstTime;
    process->waitingTime = 0;
    process->arrivalTime = time(NULL);
    process->dynamicPriority = priority;
    return process;
}

// Hauptfunktion zum Testen des Schedulers
int main() {
    PriorityQueue* queue = createPriorityQueue(10);
    
    // Beispielprozesse erstellen
    Process* p1 = createProcess(1, 5, 10);
    Process* p2 = createProcess(2, 8, 5);
    Process* p3 = createProcess(3, 3, 8);
    Process* p4 = createProcess(4, 6, 3);

    // Prozesse zur Queue hinzufügen
    enqueue(queue, p1);
    enqueue(queue, p2);
    enqueue(queue, p3);
    enqueue(queue, p4);

    // Scheduling-Simulation
    printf("Scheduling-Simulation startet...\n\n");
    
    while (queue->size > 0) {
        Process* currentProcess = dequeue(queue);
        
        printf("Prozess %d wird ausgeführt:\n", currentProcess->id);
        printf("Basis-Priorität: %d\n", currentProcess->basePriority);
        printf("Dynamische Priorität: %d\n", currentProcess->dynamicPriority);
        printf("Burst-Zeit: %d\n", currentProcess->burstTime);
        printf("Wartezeit: %d\n\n", currentProcess->waitingTime);

        // Simuliere Prozessausführung
        currentProcess->burstTime--;
        
        // Wenn der Prozess noch nicht beendet ist, füge ihn wieder zur Queue hinzu
        if (currentProcess->burstTime > 0) {
            enqueue(queue, currentProcess);
        } else {
            free(currentProcess);
        }

        // Aktualisiere Wartezeiten der wartenden Prozesse
        updateWaitingTimes(queue);
    }

    // Aufräumen
    free(queue->processes);
    free(queue);

    return 0;
}