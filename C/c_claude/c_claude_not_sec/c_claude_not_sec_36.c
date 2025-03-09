#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACK_SIZE (64 * 1024)  // 64KB Stack-Größe

// Grundlegende Coroutinen-Struktur
struct Coroutine {
    void* stack;
    ucontext_t context;
    int state;
    void (*function)(void*);
    void* arg;
    struct Coroutine* next;
};

// Funktionsprototypen
struct Coroutine* coroutine_create(void (*func)(void*), void* arg);
void coroutine_resume(struct Coroutine* co);
void coroutine_destroy(struct Coroutine* co);

// Implementierung: Coroutine erstellen
struct Coroutine* coroutine_create(void (*func)(void*), void* arg) {
    struct Coroutine* co = malloc(sizeof(struct Coroutine));
    if (!co) return NULL;
    
    co->stack = malloc(STACK_SIZE);
    if (!co->stack) {
        free(co);
        return NULL;
    }
    
    co->function = func;
    co->arg = arg;
    co->state = 0;
    co->next = NULL;
    
    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = STACK_SIZE;
    co->context.uc_link = NULL;
    
    makecontext(&co->context, (void (*)())func, 1, arg);
    
    return co;
}

// Implementierung: Coroutine ausführen
void coroutine_resume(struct Coroutine* co) {
    if (!co || co->state < 0) return;
    swapcontext(&co->context, &co->context);
}

// Implementierung: Coroutine beenden und aufräumen
void coroutine_destroy(struct Coroutine* co) {
    if (!co) return;
    if (co->stack) {
        free(co->stack);
    }
    free(co);
}

// Beispielfunktion für die Coroutine
void example_function(void* arg) {
    int* value = (int*)arg;
    printf("Coroutine started with value: %d\n", *value);
    printf("Coroutine is doing some work...\n");
    for(int i = 0; i < 3; i++) {
        printf("Coroutine iteration %d\n", i);
    }
    printf("Coroutine finished\n");
}

// Hauptprogramm zum Testen
int main() {
    printf("Main: Starting program\n");
    
    // Testwert erstellen
    int value = 42;
    
    // Coroutine erstellen
    struct Coroutine* co = coroutine_create(example_function, &value);
    if (!co) {
        printf("Failed to create coroutine\n");
        return 1;
    }
    
    printf("Main: Coroutine created, now resuming it\n");
    
    // Coroutine ausführen
    coroutine_resume(co);
    
    printf("Main: Coroutine finished, cleaning up\n");
    
    // Aufräumen
    coroutine_destroy(co);
    
    printf("Main: Program finished\n");
    return 0;
}