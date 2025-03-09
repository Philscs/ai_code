#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

// Coroutine-Struktur
typedef struct Coroutine {
    void* stack;
    jmp_buf context;
    int state;
    void (*function)(struct Coroutine*);
    struct Coroutine* next;
} Coroutine;

// Mögliche Zustände einer Coroutine
enum CoroutineState {
    COROUTINE_READY,
    COROUTINE_RUNNING,
    COROUTINE_SUSPENDED,
    COROUTINE_DONE
};

// Funktion zum Erstellen einer neuen Coroutine
Coroutine* coroutine_create(void (*function)(Coroutine*)) {
    Coroutine* co = (Coroutine*)malloc(sizeof(Coroutine));
    if (!co) {
        perror("Failed to allocate memory for Coroutine");
        exit(EXIT_FAILURE);
    }

    co->stack = NULL; // Optional: Implementierung eines benutzerdefinierten Stacks
    co->state = COROUTINE_READY;
    co->function = function;
    co->next = NULL;
    return co;
}

// Coroutine starten
void coroutine_start(Coroutine* co) {
    if (co->state == COROUTINE_READY) {
        if (setjmp(co->context) == 0) {
            co->state = COROUTINE_RUNNING;
            co->function(co);
            co->state = COROUTINE_DONE;
        }
    }
}

// Coroutine anhalten
void coroutine_yield(Coroutine* co, Coroutine* main_co) {
    co->state = COROUTINE_SUSPENDED;
    if (setjmp(co->context) == 0) {
        longjmp(main_co->context, 1);
    }
}

// Coroutine fortsetzen
void coroutine_resume(Coroutine* co) {
    if (co->state == COROUTINE_SUSPENDED) {
        co->state = COROUTINE_RUNNING;
        longjmp(co->context, 1);
    }
}

// Coroutine zerstören
void coroutine_destroy(Coroutine* co) {
    free(co);
}

// Beispiel-Coroutine-Funktion
void example_coroutine_function(Coroutine* co) {
    printf("Coroutine started\n");
    Coroutine* main_co = co->next; // Hauptcoroutine oder Scheduler

    coroutine_yield(co, main_co);
    printf("Coroutine resumed\n");

    coroutine_yield(co, main_co);
    printf("Coroutine finished\n");
}

int main() {
    Coroutine main_co;
    if (setjmp(main_co.context) == 0) {
        Coroutine* co = coroutine_create(example_coroutine_function);
        co->next = &main_co; // Referenz auf Hauptcoroutine

        coroutine_start(co);
        while (co->state != COROUTINE_DONE) {
            coroutine_resume(co);
        }

        coroutine_destroy(co);
    }

    return 0;
}
