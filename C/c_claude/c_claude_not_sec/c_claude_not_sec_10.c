#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define ORDER 4  // Ordnung des B-Baums
#define MAX_KEYS (ORDER - 1)
#define MAX_CHILDREN ORDER
#define MIN_KEYS (ORDER / 2 - 1)

// Transaktionsstatus
typedef enum {
    ACTIVE,
    COMMITTED,
    ABORTED
} TransactionStatus;

// Transaktionsprotokoll-Eintrag
typedef struct LogEntry {
    int transaction_id;
    void* old_value;
    void* new_value;
    struct LogEntry* next;
} LogEntry;

// Transaktionskontext
typedef struct {
    int transaction_id;
    TransactionStatus status;
    LogEntry* log_entries;
} Transaction;

// B-Tree Knoten
typedef struct BTreeNode {
    int* keys;
    struct BTreeNode** children;
    void** values;
    int num_keys;
    bool is_leaf;
    int transaction_id;  // Für Sperren
} BTreeNode;

// B-Tree
typedef struct {
    BTreeNode* root;
    Transaction* active_transactions;
    int next_transaction_id;
} BTree;

// Initialisierung eines neuen B-Tree Knotens
BTreeNode* create_node(bool is_leaf) {
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    node->keys = (int*)malloc(sizeof(int) * MAX_KEYS);
    node->children = (BTreeNode**)malloc(sizeof(BTreeNode*) * MAX_CHILDREN);
    node->values = (void**)malloc(sizeof(void*) * MAX_KEYS);
    node->num_keys = 0;
    node->is_leaf = is_leaf;
    node->transaction_id = -1;
    return node;
}

// Initialisierung eines neuen B-Trees
BTree* create_btree() {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    tree->root = create_node(true);
    tree->active_transactions = NULL;
    tree->next_transaction_id = 0;
    return tree;
}

// Neue Transaktion starten
Transaction* begin_transaction(BTree* tree) {
    Transaction* transaction = (Transaction*)malloc(sizeof(Transaction));
    transaction->transaction_id = tree->next_transaction_id++;
    transaction->status = ACTIVE;
    transaction->log_entries = NULL;
    return transaction;
}

// Hilfsfunktion: Protokollieren einer Änderung
void log_change(Transaction* transaction, void* old_value, void* new_value) {
    LogEntry* entry = (LogEntry*)malloc(sizeof(LogEntry));
    entry->transaction_id = transaction->transaction_id;
    entry->old_value = old_value;
    entry->new_value = new_value;
    entry->next = transaction->log_entries;
    transaction->log_entries = entry;
}

// Transaktion bestätigen (Commit)
bool commit_transaction(BTree* tree, Transaction* transaction) {
    if (transaction->status != ACTIVE) {
        return false;
    }
    
    // Hier würden wir die Änderungen permanent machen
    // und Write-Ahead-Logging durchführen
    
    transaction->status = COMMITTED;
    
    // Protokolleinträge aufräumen
    LogEntry* current = transaction->log_entries;
    while (current != NULL) {
        LogEntry* next = current->next;
        free(current);
        current = next;
    }
    
    return true;
}

// Transaktion abbrechen (Rollback)
void rollback_transaction(BTree* tree, Transaction* transaction) {
    if (transaction->status != ACTIVE) {
        return;
    }
    
    // Änderungen rückgängig machen
    LogEntry* current = transaction->log_entries;
    while (current != NULL) {
        // Hier würden wir die alten Werte wiederherstellen
        current = current->next;
    }
    
    transaction->status = ABORTED;
}

// Suchen eines Schlüssels
void* search(BTreeNode* node, int key) {
    int i = 0;
    while (i < node->num_keys && key > node->keys[i]) {
        i++;
    }
    
    if (i < node->num_keys && key == node->keys[i]) {
        return node->values[i];
    }
    
    if (node->is_leaf) {
        return NULL;
    }
    
    return search(node->children[i], key);
}

// Einfügen eines Schlüssel-Wert-Paares mit Transaktionsunterstützung
bool insert(BTree* tree, Transaction* transaction, int key, void* value) {
    if (transaction->status != ACTIVE) {
        return false;
    }
    
    BTreeNode* root = tree->root;
    
    // Wenn der Root-Knoten voll ist, splitten wir ihn
    if (root->num_keys == MAX_KEYS) {
        BTreeNode* new_root = create_node(false);
        tree->root = new_root;
        new_root->children[0] = root;
        split_child(new_root, 0, root, transaction);
        insert_non_full(new_root, key, value, transaction);
    } else {
        insert_non_full(root, key, value, transaction);
    }
    
    return true;
}

// Hilfsfunktion: Einfügen in nicht-vollen Knoten
void insert_non_full(BTreeNode* node, int key, void* value, Transaction* transaction) {
    int i = node->num_keys - 1;
    
    if (node->is_leaf) {
        // Verschiebe alle größeren Schlüssel nach rechts
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        
        // Füge den neuen Schlüssel ein
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->num_keys++;
        
        // Protokolliere die Änderung
        log_change(transaction, NULL, value);
    } else {
        // Finde den richtigen Kind-Knoten
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        
        if (node->children[i]->num_keys == MAX_KEYS) {
            split_child(node, i, node->children[i], transaction);
            if (key > node->keys[i]) {
                i++;
            }
        }
        insert_non_full(node->children[i], key, value, transaction);
    }
}

// Hilfsfunktion: Splitten eines vollen Kindknotens
void split_child(BTreeNode* parent, int index, BTreeNode* child, Transaction* transaction) {
    BTreeNode* new_child = create_node(child->is_leaf);
    new_child->num_keys = MIN_KEYS;
    
    // Kopiere die obere Hälfte der Schlüssel in den neuen Knoten
    for (int i = 0; i < MIN_KEYS; i++) {
        new_child->keys[i] = child->keys[i + MIN_KEYS + 1];
        new_child->values[i] = child->values[i + MIN_KEYS + 1];
    }
    
    // Wenn es kein Blattknoten ist, verschiebe auch die Kindzeiger
    if (!child->is_leaf) {
        for (int i = 0; i <= MIN_KEYS; i++) {
            new_child->children[i] = child->children[i + MIN_KEYS + 1];
        }
    }
    
    child->num_keys = MIN_KEYS;
    
    // Verschiebe die Elternknoten, um Platz für den neuen Schlüssel zu machen
    for (int i = parent->num_keys; i > index; i--) {
        parent->children[i + 1] = parent->children[i];
        parent->keys[i] = parent->keys[i - 1];
        parent->values[i] = parent->values[i - 1];
    }
    
    // Füge den mittleren Schlüssel in den Elternknoten ein
    parent->children[index + 1] = new_child;
    parent->keys[index] = child->keys[MIN_KEYS];
    parent->values[index] = child->values[MIN_KEYS];
    parent->num_keys++;
    
    // Protokolliere die strukturelle Änderung
    log_change(transaction, NULL, NULL);
}

// Freigeben des Speichers eines B-Tree Knotens
void free_node(BTreeNode* node) {
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            free_node(node->children[i]);
        }
    }
    free(node->keys);
    free(node->children);
    free(node->values);
    free(node);
}

// Freigeben des gesamten B-Trees
void free_btree(BTree* tree) {
    free_node(tree->root);
    free(tree);
}