#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define STACK_SIZE 1024 * 1024

struct Coroutine {
    ucontext_t context;
    int state;
};

struct CoroutineScheduler {
    struct Coroutine* coroutines;
    int numCoroutines;
    int currentCoroutine;
};

void coroutineFunction(void* arg) {
    struct Coroutine* coroutine = (struct Coroutine*)arg;
    coroutine->state = 1;

    // Coroutine logic goes here
    printf("Coroutine is running\n");
    coroutine->state = 0;
    swapcontext(&coroutine->context, &coroutine->context);
    printf("Coroutine is exiting\n");

    coroutine->state = 0;
    swapcontext(&coroutine->context, &coroutine->context);
}

void initCoroutine(struct Coroutine* coroutine, void (*function)(void*), void* arg) {
    coroutine->state = 0;
    getcontext(&coroutine->context);
    coroutine->context.uc_stack.ss_sp = malloc(STACK_SIZE);
    coroutine->context.uc_stack.ss_size = STACK_SIZE;
    coroutine->context.uc_link = NULL;
    makecontext(&coroutine->context, (void (*)(void))function, 1, arg);
}

void initCoroutineScheduler(struct CoroutineScheduler* scheduler, int numCoroutines) {
    scheduler->coroutines = (struct Coroutine*)malloc(numCoroutines * sizeof(struct Coroutine));
    scheduler->numCoroutines = numCoroutines;
    scheduler->currentCoroutine = 0;
}

void scheduleCoroutine(struct CoroutineScheduler* scheduler) {
    struct Coroutine* currentCoroutine = &scheduler->coroutines[scheduler->currentCoroutine];
    if (currentCoroutine->state == 0) {
        scheduler->currentCoroutine = (scheduler->currentCoroutine + 1) % scheduler->numCoroutines;
        struct Coroutine* nextCoroutine = &scheduler->coroutines[scheduler->currentCoroutine];
        swapcontext(&currentCoroutine->context, &nextCoroutine->context);
    }
}

int main() {
    struct CoroutineScheduler scheduler;
    initCoroutineScheduler(&scheduler, 2);

    struct Coroutine coroutine1;
    initCoroutine(&coroutine1, coroutineFunction, &coroutine1);
    scheduler.coroutines[0] = coroutine1;

    struct Coroutine coroutine2;
    initCoroutine(&coroutine2, coroutineFunction, &coroutine2);
    scheduler.coroutines[1] = coroutine2;

    while (1) {
        scheduleCoroutine(&scheduler);
    }

    return 0;
}
