#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

// Maximale Anzahl von Transaktionen und Variablen
#define MAX_TRANSACTIONS 100
#define MAX_VARIABLES 1000

// Transaktionsstatus
typedef enum {
    ACTIVE,
    COMMITTED,
    ABORTED
} TransactionStatus;

// Struktur für Variablenversion
typedef struct {
    int value;
    int version;
    pthread_mutex_t lock;
} TVar;

// Struktur für Lese-/Schreibset
typedef struct {
    int var_id;
    int read_version;
    int new_value;
} LogEntry;

// Struktur für Transaktion
typedef struct {
    int transaction_id;
    TransactionStatus status;
    LogEntry* read_set;
    int read_set_size;
    LogEntry* write_set;
    int write_set_size;
} Transaction;

// Globale Variablen
TVar variables[MAX_VARIABLES];
Transaction transactions[MAX_TRANSACTIONS];
pthread_mutex_t global_lock;
int next_transaction_id = 0;

// Initialisierung des STM-Systems
void stm_init() {
    pthread_mutex_init(&global_lock, NULL);
    for (int i = 0; i < MAX_VARIABLES; i++) {
        variables[i].value = 0;
        variables[i].version = 0;
        pthread_mutex_init(&variables[i].lock, NULL);
    }
}

// Neue Transaktion starten
Transaction* stm_begin_transaction() {
    pthread_mutex_lock(&global_lock);
    int tx_id = next_transaction_id++;
    pthread_mutex_unlock(&global_lock);

    Transaction* tx = &transactions[tx_id];
    tx->transaction_id = tx_id;
    tx->status = ACTIVE;
    tx->read_set = malloc(sizeof(LogEntry) * MAX_VARIABLES);
    tx->write_set = malloc(sizeof(LogEntry) * MAX_VARIABLES);
    tx->read_set_size = 0;
    tx->write_set_size = 0;

    return tx;
}

// Wert aus einer Variable lesen
int stm_read(Transaction* tx, int var_id) {
    // Prüfen ob Variable bereits im Write-Set
    for (int i = 0; i < tx->write_set_size; i++) {
        if (tx->write_set[i].var_id == var_id) {
            return tx->write_set[i].new_value;
        }
    }

    // Variable lesen und zum Read-Set hinzufügen
    pthread_mutex_lock(&variables[var_id].lock);
    int current_value = variables[var_id].value;
    int current_version = variables[var_id].version;
    pthread_mutex_unlock(&variables[var_id].lock);

    LogEntry* entry = &tx->read_set[tx->read_set_size++];
    entry->var_id = var_id;
    entry->read_version = current_version;

    return current_value;
}

// Wert in eine Variable schreiben
void stm_write(Transaction* tx, int var_id, int value) {
    LogEntry* entry = &tx->write_set[tx->write_set_size++];
    entry->var_id = var_id;
    entry->new_value = value;
}

// Validierung der Transaktion
bool validate_transaction(Transaction* tx) {
    // Prüfen ob alle gelesenen Versionen noch aktuell sind
    for (int i = 0; i < tx->read_set_size; i++) {
        int var_id = tx->read_set[i].var_id;
        pthread_mutex_lock(&variables[var_id].lock);
        if (variables[var_id].version != tx->read_set[i].read_version) {
            pthread_mutex_unlock(&variables[var_id].lock);
            return false;
        }
        pthread_mutex_unlock(&variables[var_id].lock);
    }
    return true;
}

// Transaktion committen
bool stm_commit(Transaction* tx) {
    // Alle benötigten Locks aquirieren
    for (int i = 0; i < tx->write_set_size; i++) {
        pthread_mutex_lock(&variables[tx->write_set[i].var_id].lock);
    }

    // Transaktion validieren
    if (!validate_transaction(tx)) {
        // Locks freigeben und abbrechen
        for (int i = 0; i < tx->write_set_size; i++) {
            pthread_mutex_unlock(&variables[tx->write_set[i].var_id].lock);
        }
        tx->status = ABORTED;
        return false;
    }

    // Änderungen durchführen
    for (int i = 0; i < tx->write_set_size; i++) {
        int var_id = tx->write_set[i].var_id;
        variables[var_id].value = tx->write_set[i].new_value;
        variables[var_id].version++;
    }

    // Locks freigeben
    for (int i = 0; i < tx->write_set_size; i++) {
        pthread_mutex_unlock(&variables[tx->write_set[i].var_id].lock);
    }

    tx->status = COMMITTED;
    return true;
}

// Transaktion aufräumen
void stm_end_transaction(Transaction* tx) {
    free(tx->read_set);
    free(tx->write_set);
}

// Beispiel zur Verwendung
void example_usage() {
    // STM initialisieren
    stm_init();

    // Transaktion starten
    Transaction* tx = stm_begin_transaction();

    // Werte lesen und schreiben
    int value = stm_read(tx, 0);
    stm_write(tx, 0, value + 1);

    // Transaktion committen
    if (stm_commit(tx)) {
        printf("Transaktion erfolgreich\n");
    } else {
        printf("Transaktion fehlgeschlagen\n");
    }

    // Aufräumen
    stm_end_transaction(tx);
}