#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

#define MAX_TRANSACTIONS 128
#define MAX_VARS 128

// Struktur für eine Variable in STM
typedef struct {
    int value;
    pthread_mutex_t lock;
} STM_Var;

// Struktur für eine Transaktion
typedef struct {
    bool active;
    STM_Var *readSet[MAX_VARS];
    STM_Var *writeSet[MAX_VARS];
    int readValues[MAX_VARS];
    int writeValues[MAX_VARS];
    int readCount;
    int writeCount;
} Transaction;

STM_Var vars[MAX_VARS];
Transaction transactions[MAX_TRANSACTIONS];

pthread_mutex_t globalLock = PTHREAD_MUTEX_INITIALIZER;

// Initialisiert die STM-Variablen
void STM_init() {
    for (int i = 0; i < MAX_VARS; i++) {
        vars[i].value = 0;
        pthread_mutex_init(&vars[i].lock, NULL);
    }
}

// Startet eine neue Transaktion
Transaction *STM_begin() {
    pthread_mutex_lock(&globalLock);
    for (int i = 0; i < MAX_TRANSACTIONS; i++) {
        if (!transactions[i].active) {
            transactions[i].active = true;
            transactions[i].readCount = 0;
            transactions[i].writeCount = 0;
            pthread_mutex_unlock(&globalLock);
            return &transactions[i];
        }
    }
    pthread_mutex_unlock(&globalLock);
    return NULL; // Keine verfügbaren Transaktionen
}

// Liest den Wert einer Variable in einer Transaktion
int STM_read(Transaction *tx, STM_Var *var) {
    for (int i = 0; i < tx->readCount; i++) {
        if (tx->readSet[i] == var) {
            return tx->readValues[i];
        }
    }

    pthread_mutex_lock(&var->lock);
    int value = var->value;
    pthread_mutex_unlock(&var->lock);

    tx->readSet[tx->readCount] = var;
    tx->readValues[tx->readCount] = value;
    tx->readCount++;

    return value;
}

// Schreibt einen Wert in einer Transaktion
void STM_write(Transaction *tx, STM_Var *var, int value) {
    for (int i = 0; i < tx->writeCount; i++) {
        if (tx->writeSet[i] == var) {
            tx->writeValues[i] = value;
            return;
        }
    }

    tx->writeSet[tx->writeCount] = var;
    tx->writeValues[tx->writeCount] = value;
    tx->writeCount++;
}

// Beendet die Transaktion und commitet die Änderungen
bool STM_commit(Transaction *tx) {
    // Überprüfe, ob die gelesenen Werte immer noch gültig sind
    for (int i = 0; i < tx->readCount; i++) {
        STM_Var *var = tx->readSet[i];
        pthread_mutex_lock(&var->lock);
        if (var->value != tx->readValues[i]) {
            pthread_mutex_unlock(&var->lock);
            STM_abort(tx);
            return false;
        }
        pthread_mutex_unlock(&var->lock);
    }

    // Schreibe die Änderungen
    for (int i = 0; i < tx->writeCount; i++) {
        STM_Var *var = tx->writeSet[i];
        pthread_mutex_lock(&var->lock);
        var->value = tx->writeValues[i];
        pthread_mutex_unlock(&var->lock);
    }

    tx->active = false;
    return true;
}

// Bricht die Transaktion ab
void STM_abort(Transaction *tx) {
    tx->active = false;
}

// Beispiel-Hauptprogramm
int main() {
    STM_init();

    Transaction *tx = STM_begin();
    if (tx == NULL) {
        printf("Fehler: Keine verfügbaren Transaktionen!\n");
        return 1;
    }

    STM_write(tx, &vars[0], 42);
    int value = STM_read(tx, &vars[0]);
    printf("Gelesener Wert: %d\n", value);

    if (STM_commit(tx)) {
        printf("Transaktion erfolgreich!\n");
    } else {
        printf("Transaktion fehlgeschlagen!\n");
    }

    return 0;
} 
