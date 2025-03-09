#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KEY_SIZE 256
#define MAX_VALUE_SIZE 1024
#define MEMTABLE_SIZE 1000
#define MAX_LEVEL 4

typedef struct {
    char key[MAX_KEY_SIZE];
    char value[MAX_VALUE_SIZE];
    int deleted;  // Tombstone flag
} Entry;

typedef struct {
    Entry* entries;
    int size;
    int capacity;
} MemTable;

typedef struct {
    Entry* entries;
    int size;
    int capacity;
    int level;
} SSTable;

typedef struct {
    MemTable* memtable;
    SSTable** sstables;  // Array of SSTables for each level
    int* sstable_counts;  // Number of SSTables at each level
} LSMTree;

// Function declarations
LSMTree* create_lsm_tree();
void free_lsm_tree(LSMTree* tree);
int put(LSMTree* tree, const char* key, const char* value);
char* get(LSMTree* tree, const char* key);
int delete(LSMTree* tree, const char* key);
void flush_memtable(LSMTree* tree);
void merge_sstables(LSMTree* tree, int level);

// Create a new LSM tree
LSMTree* create_lsm_tree() {
    LSMTree* tree = (LSMTree*)malloc(sizeof(LSMTree));
    
    // Initialize memtable
    tree->memtable = (MemTable*)malloc(sizeof(MemTable));
    tree->memtable->entries = (Entry*)malloc(MEMTABLE_SIZE * sizeof(Entry));
    tree->memtable->size = 0;
    tree->memtable->capacity = MEMTABLE_SIZE;
    
    // Initialize SSTable levels
    tree->sstables = (SSTable**)malloc(MAX_LEVEL * sizeof(SSTable*));
    tree->sstable_counts = (int*)calloc(MAX_LEVEL, sizeof(int));
    
    for (int i = 0; i < MAX_LEVEL; i++) {
        tree->sstables[i] = NULL;
    }
    
    return tree;
}

// Insert or update a key-value pair
int put(LSMTree* tree, const char* key, const char* value) {
    if (tree->memtable->size >= tree->memtable->capacity) {
        flush_memtable(tree);
    }
    
    // Check if key exists in memtable
    for (int i = 0; i < tree->memtable->size; i++) {
        if (strcmp(tree->memtable->entries[i].key, key) == 0) {
            strcpy(tree->memtable->entries[i].value, value);
            tree->memtable->entries[i].deleted = 0;
            return 0;
        }
    }
    
    // Add new entry
    Entry* entry = &tree->memtable->entries[tree->memtable->size++];
    strncpy(entry->key, key, MAX_KEY_SIZE - 1);
    strncpy(entry->value, value, MAX_VALUE_SIZE - 1);
    entry->deleted = 0;
    
    return 0;
}

// Retrieve value for a given key
char* get(LSMTree* tree, const char* key) {
    // Check memtable first
    for (int i = 0; i < tree->memtable->size; i++) {
        if (strcmp(tree->memtable->entries[i].key, key) == 0) {
            if (tree->memtable->entries[i].deleted) {
                return NULL;  // Key was deleted
            }
            return tree->memtable->entries[i].value;
        }
    }
    
    // Check SSTables from newest to oldest
    for (int level = 0; level < MAX_LEVEL; level++) {
        for (int table = 0; table < tree->sstable_counts[level]; table++) {
            SSTable* sstable = &tree->sstables[level][table];
            for (int i = 0; i < sstable->size; i++) {
                if (strcmp(sstable->entries[i].key, key) == 0) {
                    if (sstable->entries[i].deleted) {
                        return NULL;  // Key was deleted
                    }
                    return sstable->entries[i].value;
                }
            }
        }
    }
    
    return NULL;  // Key not found
}

// Mark a key as deleted
int delete(LSMTree* tree, const char* key) {
    if (tree->memtable->size >= tree->memtable->capacity) {
        flush_memtable(tree);
    }
    
    // Add deletion entry (tombstone)
    Entry* entry = &tree->memtable->entries[tree->memtable->size++];
    strncpy(entry->key, key, MAX_KEY_SIZE - 1);
    entry->value[0] = '\0';
    entry->deleted = 1;
    
    return 0;
}

// Flush memtable to Level 0 SSTable
void flush_memtable(LSMTree* tree) {
    if (tree->memtable->size == 0) {
        return;
    }
    
    // Create new SSTable
    SSTable* new_sstable = (SSTable*)malloc(sizeof(SSTable));
    new_sstable->entries = (Entry*)malloc(tree->memtable->size * sizeof(Entry));
    new_sstable->size = tree->memtable->size;
    new_sstable->capacity = tree->memtable->size;
    new_sstable->level = 0;
    
    // Copy entries from memtable
    memcpy(new_sstable->entries, tree->memtable->entries, 
           tree->memtable->size * sizeof(Entry));
    
    // Add SSTable to level 0
    tree->sstable_counts[0]++;
    SSTable* new_tables = (SSTable*)realloc(tree->sstables[0], 
                                           tree->sstable_counts[0] * sizeof(SSTable));
    if (new_tables != NULL) {
        tree->sstables[0] = new_tables;
        tree->sstables[0][tree->sstable_counts[0] - 1] = *new_sstable;
    }
    
    // Reset memtable
    tree->memtable->size = 0;
    
    // Check if we need to merge
    if (tree->sstable_counts[0] > 2) {
        merge_sstables(tree, 0);
    }
    
    free(new_sstable);
}

// Merge SSTables at given level
void merge_sstables(LSMTree* tree, int level) {
    if (level >= MAX_LEVEL - 1 || tree->sstable_counts[level] <= 2) {
        return;
    }
    
    // Create new merged SSTable for next level
    int total_entries = 0;
    for (int i = 0; i < tree->sstable_counts[level]; i++) {
        total_entries += tree->sstables[level][i].size;
    }
    
    SSTable merged_table;
    merged_table.entries = (Entry*)malloc(total_entries * sizeof(Entry));
    merged_table.size = 0;
    merged_table.capacity = total_entries;
    merged_table.level = level + 1;
    
    // Merge entries (simple concatenation for this example)
    for (int i = 0; i < tree->sstable_counts[level]; i++) {
        memcpy(&merged_table.entries[merged_table.size],
               tree->sstables[level][i].entries,
               tree->sstables[level][i].size * sizeof(Entry));
        merged_table.size += tree->sstables[level][i].size;
    }
    
    // Add merged table to next level
    tree->sstable_counts[level + 1]++;
    SSTable* new_tables = (SSTable*)realloc(tree->sstables[level + 1],
                                           tree->sstable_counts[level + 1] * sizeof(SSTable));
    if (new_tables != NULL) {
        tree->sstables[level + 1] = new_tables;
        tree->sstables[level + 1][tree->sstable_counts[level + 1] - 1] = merged_table;
    }
    
    // Clear current level
    free(tree->sstables[level]);
    tree->sstables[level] = NULL;
    tree->sstable_counts[level] = 0;
    
    // Check if we need to merge at next level
    if (tree->sstable_counts[level + 1] > 2) {
        merge_sstables(tree, level + 1);
    }
}

// Free all allocated memory
void free_lsm_tree(LSMTree* tree) {
    // Free memtable
    free(tree->memtable->entries);
    free(tree->memtable);
    
    // Free all SSTables
    for (int level = 0; level < MAX_LEVEL; level++) {
        if (tree->sstables[level] != NULL) {
            for (int table = 0; table < tree->sstable_counts[level]; table++) {
                free(tree->sstables[level][table].entries);
            }
            free(tree->sstables[level]);
        }
    }
    
    free(tree->sstables);
    free(tree->sstable_counts);
    free(tree);
}

// Example usage
int main() {
    LSMTree* tree = create_lsm_tree();
    
    // Insert some key-value pairs
    put(tree, "key1", "value1");
    put(tree, "key2", "value2");
    put(tree, "key3", "value3");
    
    // Retrieve values
    printf("key1: %s\n", get(tree, "key1"));
    printf("key2: %s\n", get(tree, "key2"));
    
    // Delete a key
    delete(tree, "key2");
    printf("key2 after deletion: %s\n", get(tree, "key2"));
    
    // Update a value
    put(tree, "key1", "new_value1");
    printf("key1 after update: %s\n", get(tree, "key1"));
    
    // Clean up
    free_lsm_tree(tree);
    
    return 0;
}