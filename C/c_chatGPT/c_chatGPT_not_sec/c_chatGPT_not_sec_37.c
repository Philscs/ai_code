#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Node structure for the in-memory tree (Memtable)
typedef struct Node {
    char *key;
    char *value;
    struct Node *left, *right;
} Node;

// Function to create a new node
Node *create_node(const char *key, const char *value) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->key = strdup(key);
    node->value = strdup(value);
    node->left = node->right = NULL;
    return node;
}

// Insert function for the binary search tree
Node *insert(Node *root, const char *key, const char *value) {
    if (root == NULL) {
        return create_node(key, value);
    }
    if (strcmp(key, root->key) < 0) {
        root->left = insert(root->left, key, value);
    } else if (strcmp(key, root->key) > 0) {
        root->right = insert(root->right, key, value);
    } else {
        // Update value if key exists
        free(root->value);
        root->value = strdup(value);
    }
    return root;
}

// Search function for the binary search tree
char *search(Node *root, const char *key) {
    if (root == NULL) {
        return NULL;
    }
    if (strcmp(key, root->key) == 0) {
        return root->value;
    } else if (strcmp(key, root->key) < 0) {
        return search(root->left, key);
    } else {
        return search(root->right, key);
    }
}

// Serialize the Memtable to an SSTable (file on disk)
void serialize_to_sstable(Node *root, FILE *file) {
    if (root == NULL) {
        return;
    }
    serialize_to_sstable(root->left, file);
    fprintf(file, "%s\t%s\n", root->key, root->value);
    serialize_to_sstable(root->right, file);
}

// Free the in-memory tree
void free_tree(Node *root) {
    if (root == NULL) {
        return;
    }
    free_tree(root->left);
    free_tree(root->right);
    free(root->key);
    free(root->value);
    free(root);
}

// Main function for demonstration
int main() {
    Node *memtable = NULL;

    // Insert key-value pairs into the Memtable
    memtable = insert(memtable, "key1", "value1");
    memtable = insert(memtable, "key2", "value2");
    memtable = insert(memtable, "key3", "value3");

    // Search for a key
    char *value = search(memtable, "key2");
    if (value) {
        printf("Found: %s\n", value);
    } else {
        printf("Key not found\n");
    }

    // Serialize Memtable to SSTable
    FILE *file = fopen("sstable.txt", "w");
    if (file) {
        serialize_to_sstable(memtable, file);
        fclose(file);
        printf("Memtable serialized to sstable.txt\n");
    } else {
        perror("Failed to open file");
    }

    // Free the Memtable
    free_tree(memtable);

    return 0;
}
