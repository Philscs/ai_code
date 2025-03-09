#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#define ORDER 4
#define MAX_KEYS (ORDER - 1)
#define MAX_CHILDREN ORDER
#define MIN_KEYS (ORDER / 2 - 1)

typedef struct {
    int key;
    void* value;
} Entry;

typedef struct BPNode {
    bool is_leaf;
    int num_keys;
    Entry entries[MAX_KEYS];
    struct BPNode* children[MAX_CHILDREN];
    struct BPNode* next;  // Für Blattverkettung
    pthread_mutex_t lock; // Für gleichzeitigen Zugriff
} BPNode;

typedef struct {
    BPNode* root;
    pthread_mutex_t tree_lock;
} BPTree;

// Checkpoint-Struktur für Recovery
typedef struct {
    int operation;  // 1: insert, 2: delete
    int key;
    void* value;
    long timestamp;
} LogRecord;

// Initialisierung
BPTree* create_tree() {
    BPTree* tree = (BPTree*)malloc(sizeof(BPTree));
    tree->root = NULL;
    pthread_mutex_init(&tree->tree_lock, NULL);
    return tree;
}

BPNode* create_node(bool is_leaf) {
    BPNode* node = (BPNode*)malloc(sizeof(BPNode));
    node->is_leaf = is_leaf;
    node->num_keys = 0;
    node->next = NULL;
    pthread_mutex_init(&node->lock, NULL);
    
    for (int i = 0; i < MAX_CHILDREN; i++) {
        node->children[i] = NULL;
    }
    return node;
}

// Hilfsfunktionen
int binary_search(Entry* entries, int num_keys, int key) {
    int left = 0;
    int right = num_keys - 1;
    
    while (left <= right) {
        int mid = (left + right) / 2;
        if (entries[mid].key == key) return mid;
        if (entries[mid].key < key) left = mid + 1;
        else right = mid - 1;
    }
    return left;
}

// Einfügen
void insert_in_node(BPNode* node, int key, void* value, BPNode* right_child) {
    int pos = binary_search(node->entries, node->num_keys, key);
    
    for (int i = node->num_keys; i > pos; i--) {
        node->entries[i] = node->entries[i-1];
        if (!node->is_leaf) {
            node->children[i+1] = node->children[i];
        }
    }
    
    node->entries[pos].key = key;
    node->entries[pos].value = value;
    if (!node->is_leaf) {
        node->children[pos+1] = right_child;
    }
    node->num_keys++;
}

BPNode* split_node(BPNode* node) {
    int mid = node->num_keys / 2;
    BPNode* new_node = create_node(node->is_leaf);
    
    for (int i = mid; i < node->num_keys; i++) {
        new_node->entries[new_node->num_keys] = node->entries[i];
        if (!node->is_leaf) {
            new_node->children[new_node->num_keys+1] = node->children[i+1];
        }
        new_node->num_keys++;
    }
    
    if (node->is_leaf) {
        new_node->next = node->next;
        node->next = new_node;
    }
    
    node->num_keys = mid;
    return new_node;
}

// Haupteinfügeoperation mit Unterstützung für gleichzeitigen Zugriff
void insert(BPTree* tree, int key, void* value) {
    pthread_mutex_lock(&tree->tree_lock);
    
    if (tree->root == NULL) {
        tree->root = create_node(true);
        insert_in_node(tree->root, key, value, NULL);
        pthread_mutex_unlock(&tree->tree_lock);
        return;
    }
    
    BPNode* current = tree->root;
    BPNode* parent = NULL;
    
    // Pfad zum Blatt sperren
    while (!current->is_leaf) {
        pthread_mutex_lock(&current->lock);
        if (parent) pthread_mutex_unlock(&parent->lock);
        parent = current;
        
        int pos = binary_search(current->entries, current->num_keys, key);
        current = current->children[pos];
    }
    
    pthread_mutex_lock(&current->lock);
    
    // Einfügen und Split falls nötig
    if (current->num_keys < MAX_KEYS) {
        insert_in_node(current, key, value, NULL);
        pthread_mutex_unlock(&current->lock);
        if (parent) pthread_mutex_unlock(&parent->lock);
        pthread_mutex_unlock(&tree->tree_lock);
        return;
    }
    
    // Split erforderlich
    BPNode* new_node = split_node(current);
    int promote_key = new_node->entries[0].key;
    
    // Rekursives Aufsteigen und Splits
    while (parent && parent->num_keys == MAX_KEYS) {
        BPNode* new_parent = split_node(parent);
        promote_key = parent->entries[parent->num_keys-1].key;
        parent = new_parent;
    }
    
    // Wurzelsplit
    if (!parent) {
        parent = create_node(false);
        insert_in_node(parent, promote_key, NULL, new_node);
        parent->children[0] = current;
        tree->root = parent;
    }
    
    pthread_mutex_unlock(&current->lock);
    if (parent) pthread_mutex_unlock(&parent->lock);
    pthread_mutex_unlock(&tree->tree_lock);
}

// Suchen
void* search(BPTree* tree, int key) {
    if (!tree->root) return NULL;
    
    BPNode* current = tree->root;
    while (!current->is_leaf) {
        int pos = binary_search(current->entries, current->num_keys, key);
        current = current->children[pos];
    }
    
    int pos = binary_search(current->entries, current->num_keys, key);
    if (pos < current->num_keys && current->entries[pos].key == key) {
        return current->entries[pos].value;
    }
    return NULL;
}

// Recovery Management
void write_log(LogRecord* record) {
    FILE* log_file = fopen("bptree.log", "ab");
    if (log_file) {
        fwrite(record, sizeof(LogRecord), 1, log_file);
        fclose(log_file);
    }
}

void recover_from_log() {
    FILE* log_file = fopen("bptree.log", "rb");
    if (!log_file) return;
    
    LogRecord record;
    while (fread(&record, sizeof(LogRecord), 1, log_file) == 1) {
        // Wiederherstellen der Operationen
        if (record.operation == 1) {  // Insert
            // insert(tree, record.key, record.value);
        }
        // Weitere Recovery-Logik hier
    }
    fclose(log_file);
}

// Optimierter Bereichsabfragen
typedef struct {
    int start_key;
    int end_key;
    void** results;
    int count;
} RangeQuery;

void range_search(BPTree* tree, RangeQuery* query) {
    if (!tree->root) return;
    
    // Finde das Startblatt
    BPNode* current = tree->root;
    while (!current->is_leaf) {
        int pos = binary_search(current->entries, current->num_keys, query->start_key);
        current = current->children[pos];
    }
    
    // Traversiere die Blätter
    while (current && current->entries[0].key <= query->end_key) {
        for (int i = 0; i < current->num_keys; i++) {
            if (current->entries[i].key >= query->start_key && 
                current->entries[i].key <= query->end_key) {
                query->results[query->count++] = current->entries[i].value;
            }
        }
        current = current->next;
    }
}

// Aufräumen
void free_node(BPNode* node) {
    if (!node) return;
    
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            free_node(node->children[i]);
        }
    }
    
    pthread_mutex_destroy(&node->lock);
    free(node);
}

void free_tree(BPTree* tree) {
    if (tree) {
        free_node(tree->root);
        pthread_mutex_destroy(&tree->tree_lock);
        free(tree);
    }
}