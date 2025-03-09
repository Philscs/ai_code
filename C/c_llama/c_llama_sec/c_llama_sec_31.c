#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Typen
typedef struct {
    pid_t process_id;
    bool running;
} Process;

// Funktionen
void* process_function(void* arg) {
    Process* process = (Process*)arg;
    while (process->running) {
        // Simuliere die Ausführung eines Prozesses
        printf(".\n");
        usleep(100000);
    }
    return NULL;
}

void* task_function(void* arg) {
    Process* process = (Process*)arg;
    while (process->running) {
        // Simuliere die Ausführung einer Task
        printf("Task gestartet!\n");
        // Simuliere eine kleine Pause
        usleep(500000);
        printf("Task beendet!\n");
    }
    return NULL;
}

void stop_process(Process* process) {
    kill(process->process_id, SIGTERM);
    sleep(1); // Warte für die Fortschreibung der Signalverarbeitung
    waitpid(process->process_id, NULL, 0);
    process->running = false;
}

int main() {
    int max_threads = 5;
    int min_run_time = 1000;
    float max_priority = 1.0f;

    Process processes[max_threads];
    pthread_t threads[max_threads];

    // Initialisierung der Prozesse
    for (int i = 0; i < max_threads; i++) {
        pid_t process_id = fork();
        if (process_id == -1) {
            perror("fork");
            return -1;
        }
        if (process_id == 0) { // Kind
            processes[i].running = true;
            // Simuliere die Ausführung eines Prozesses
            for (int j = 0; j < min_run_time / 10; j++) {
                printf(".\n");
                usleep(100000);
            }
        } else { // Elternteil
            processes[i].process_id = process_id;
            processes[i].running = false;
        }
    }

    // Initialisierung der Tasks
    for (int i = 0; i < max_threads; i++) {
        pthread_create(&threads[i], NULL, task_function, &processes[i]);
    }

    // Stoppen des Schedulers
    for (int i = 0; i < max_threads; i++) {
        stop_process(&processes[i]);
    }

    return 0;
}