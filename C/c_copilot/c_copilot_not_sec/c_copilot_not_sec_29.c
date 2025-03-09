#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_TRANSACTIONS 100
#define MAX_OPERATIONS 100

typedef struct {
    int* memory;
    bool* readSet;
    bool* writeSet;
    int readSetSize;
    int writeSetSize;
} Transaction;

Transaction transactions[MAX_TRANSACTIONS];
int transactionCount = 0;

void stm_init() {
    for (int i = 0; i < MAX_TRANSACTIONS; i++) {
        transactions[i].memory = (int*)malloc(sizeof(int) * MAX_OPERATIONS);
        transactions[i].readSet = (bool*)malloc(sizeof(bool) * MAX_OPERATIONS);
        transactions[i].writeSet = (bool*)malloc(sizeof(bool) * MAX_OPERATIONS);
        transactions[i].readSetSize = 0;
        transactions[i].writeSetSize = 0;
    }
}

void stm_destroy() {
    for (int i = 0; i < MAX_TRANSACTIONS; i++) {
        free(transactions[i].memory);
        free(transactions[i].readSet);
        free(transactions[i].writeSet);
    }
}

void stm_start_transaction() {
    if (transactionCount >= MAX_TRANSACTIONS) {
        printf("Maximum transaction limit reached.\n");
        return;
    }

    transactions[transactionCount].readSetSize = 0;
    transactions[transactionCount].writeSetSize = 0;
    transactionCount++;
}

void stm_read(int* addr) {
    Transaction* currentTransaction = &transactions[transactionCount - 1];
    currentTransaction->memory[currentTransaction->readSetSize] = *addr;
    currentTransaction->readSet[currentTransaction->readSetSize] = true;
    currentTransaction->readSetSize++;
}

void stm_write(int* addr, int value) {
    Transaction* currentTransaction = &transactions[transactionCount - 1];
    currentTransaction->memory[currentTransaction->writeSetSize] = value;
    currentTransaction->writeSet[currentTransaction->writeSetSize] = true;
    currentTransaction->writeSetSize++;
}

void stm_commit() {
    Transaction* currentTransaction = &transactions[transactionCount - 1];

    for (int i = 0; i < currentTransaction->writeSetSize; i++) {
        if (currentTransaction->writeSet[i]) {
            *(currentTransaction->memory + i) = currentTransaction->memory[i];
        }
    }

    transactionCount--;
}

int main() {
    stm_init();

    int sharedVariable = 0;

    stm_start_transaction();
    stm_read(&sharedVariable);
    stm_write(&sharedVariable, 10);
    stm_commit();

    printf("Shared variable value: %d\n", sharedVariable);

    stm_destroy();

    return 0;
}
