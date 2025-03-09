#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// Strukt für einen Task
typedef struct {
    char Name[256];
    int Priority;
    int Deadline;
    bool Finished;
} Task;

// Struktur für die Priorität-Inherenz
typedef struct {
    bool IsInherited;
} PriorityInheritance;

// Struktur für die Ressourcenverwaltung
typedef struct {
    int Available;
    int Used;
} Resources;

// Prioritäten-Erstellung-Liste (Prio-Liste)
typedef struct {
    Task* Tasks;
    int Size;
    int Capacity;
} PrioList;

// Funktion zur Erstellung einer Prio-Liste
PrioList* createPrioList(int capacity) {
    PrioList* prioList = malloc(sizeof(PrioList));
    prioList->Tasks = malloc(sizeof(Task) * capacity);
    prioList->Size = 0;
    prioList->Capacity = capacity;
    return prioList;
}

// Funktion zur Hinzufügung eines Tasks in die Prio-Liste
void addTask(PrioList* prioList, Task task) {
    if (prioList->Size < prioList->Capacity) {
        prioList->Tasks[prioList->Size] = task;
        prioList->Size++;
    } else {
        printf("Prio-Liste voll! Neuer Task nicht hinzufügen.\n");
    }
}

// Funktion zur Ausführung der Prio-Liste
void executePrioList(PrioList* prioList) {
    while (prioList->Size > 0) {
        Task currentTask = prioList->Tasks[0];
        
        // Checken, ob der Frist abgelaufen ist
        if (currentTask.Deadline <= time(NULL)) {
            printf("Frist abgelaufen! %s\n", currentTask.Name);
            currentTask.Finished = true;
        } else if (currentTask.Finished) {
            // Prioritätserbschaft: Warten, bis die Vorgänger beendet sind
            for (int i = 0; i < prioList->Size - 1; i++) {
                Task nextTask = prioList->Tasks[i + 1];
                
                if (!nextTask.Finished) {
                    printf("Prioritätserbschaft: %s wartet auf %s\n", currentTask.Name, 
nextTask.Name);
                    break;
                }
            }
        } else {
            // Ersetzen des aktuellen Tasks
            for (int i = 1; i < prioList->Size; i++) {
                prioList->Tasks[i - 1] = prioList->Tasks[i];
            }
            prioList->Size--;
            
            // Zeit für den nächsten Task vergeben
            clock_t start = clock();
            while (clock() - start < currentTask.Deadline) {}
        }
    }
}

// Funktion zur Ressourcenverwaltung
void manageResources(Resources* resources, int amount) {
    if (resources->Used + amount <= resources->Available) {
        resources->Used += amount;
        printf("Ressourcen vergeben: %d\n", amount);
    } else {
        printf("Keine Ressourcen mehr verfügbar!\n");
    }
}

int main() {
    // Prio-Liste erzeugen
    PrioList* prioList = createPrioList(10);

    // Tasks erstellen und hinzufügen
    Task task1;
    strcpy(task1.Name, "Task 1");
    task1.Priority = 5;
    task1.Deadline = time(NULL) + 2;
    addTask(prioList, task1);

    Task task2;
    strcpy(task2.Name, "Task 2");
    task2.Priority = 3;
    task2.Deadline = time(NULL) + 4;
    addTask(prioList, task2);

    // Ressourcen verwalten
    Resources resources;
    resources.Available = 10;
    resources.Used = 0;
    manageResources(&resources, 5);

    // Prio-Liste ausführen
    executePrioList(prioList);
    return 0;
}
