#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <time.h>

#define RED   0
#define BLACK 1
#define AUDIT_LOG_FILE "rbtree_audit.log"

typedef struct Node {
    int key;
    int color;
    struct Node *left;
    struct Node *right;
    struct Node *parent;
    atomic_int ref_count;
} Node;

typedef struct {
    Node *root;
    pthread_mutex_t lock;
    FILE *audit_log;
    pthread_mutex_t log_lock;
} RBTree;

// Audit logging functions
static void log_operation(RBTree *tree, const char *operation, int key) {
    pthread_mutex_lock(&tree->log_lock);
    
    time_t now;
    time(&now);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';  // Remove newline
    
    fprintf(tree->audit_log, "[%s] %s: key=%d\n", timestamp, operation, key);
    fflush(tree->audit_log);
    
    pthread_mutex_unlock(&tree->log_lock);
}

// Reference counting functions
static void increment_ref(Node *node) {
    if (node) {
        atomic_fetch_add(&node->ref_count, 1);
    }
}

static void decrement_ref(Node *node) {
    if (node && atomic_fetch_sub(&node->ref_count, 1) == 1) {
        decrement_ref(node->left);
        decrement_ref(node->right);
        free(node);
    }
}

// Tree initialization
RBTree* rb_tree_create(void) {
    RBTree *tree = (RBTree*)malloc(sizeof(RBTree));
    if (!tree) return NULL;
    
    tree->root = NULL;
    pthread_mutex_init(&tree->lock, NULL);
    pthread_mutex_init(&tree->log_lock, NULL);
    
    tree->audit_log = fopen(AUDIT_LOG_FILE, "a");
    if (!tree->audit_log) {
        pthread_mutex_destroy(&tree->lock);
        pthread_mutex_destroy(&tree->log_lock);
        free(tree);
        return NULL;
    }
    
    return tree;
}

// Helper functions for rotations
static void left_rotate(RBTree *tree, Node *x) {
    Node *y = x->right;
    x->right = y->left;
    
    if (y->left)
        y->left->parent = x;
    
    y->parent = x->parent;
    
    if (!x->parent)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    
    y->left = x;
    x->parent = y;
}

static void right_rotate(RBTree *tree, Node *y) {
    Node *x = y->left;
    y->left = x->right;
    
    if (x->right)
        x->right->parent = y;
    
    x->parent = y->parent;
    
    if (!y->parent)
        tree->root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;
    
    x->right = y;
    y->parent = x;
}

// Insertion function
void rb_tree_insert(RBTree *tree, int key) {
    pthread_mutex_lock(&tree->lock);
    
    // Create new node
    Node *node = (Node*)malloc(sizeof(Node));
    if (!node) {
        pthread_mutex_unlock(&tree->lock);
        return;
    }
    
    node->key = key;
    node->color = RED;
    node->left = node->right = node->parent = NULL;
    atomic_init(&node->ref_count, 1);
    
    // Standard BST insertion
    Node *y = NULL;
    Node *x = tree->root;
    
    while (x) {
        y = x;
        if (key < x->key)
            x = x->left;
        else
            x = x->right;
    }
    
    node->parent = y;
    if (!y)
        tree->root = node;
    else if (key < y->key)
        y->left = node;
    else
        y->right = node;
    
    // Fix Red-Black properties
    Node *current = node;
    while (current != tree->root && current->parent->color == RED) {
        if (current->parent == current->parent->parent->left) {
            Node *uncle = current->parent->parent->right;
            
            if (uncle && uncle->color == RED) {
                current->parent->color = BLACK;
                uncle->color = BLACK;
                current->parent->parent->color = RED;
                current = current->parent->parent;
            } else {
                if (current == current->parent->right) {
                    current = current->parent;
                    left_rotate(tree, current);
                }
                current->parent->color = BLACK;
                current->parent->parent->color = RED;
                right_rotate(tree, current->parent->parent);
            }
        } else {
            // Same as above with "left" and "right" exchanged
            Node *uncle = current->parent->parent->left;
            
            if (uncle && uncle->color == RED) {
                current->parent->color = BLACK;
                uncle->color = BLACK;
                current->parent->parent->color = RED;
                current = current->parent->parent;
            } else {
                if (current == current->parent->left) {
                    current = current->parent;
                    right_rotate(tree, current);
                }
                current->parent->color = BLACK;
                current->parent->parent->color = RED;
                left_rotate(tree, current->parent->parent);
            }
        }
    }
    
    tree->root->color = BLACK;
    log_operation(tree, "INSERT", key);
    pthread_mutex_unlock(&tree->lock);
}

// Serialization functions
static void serialize_node(Node *node, FILE *file) {
    if (!node) {
        fwrite(&(int){-1}, sizeof(int), 1, file);
        return;
    }
    
    fwrite(&node->key, sizeof(int), 1, file);
    fwrite(&node->color, sizeof(int), 1, file);
    serialize_node(node->left, file);
    serialize_node(node->right, file);
}

bool rb_tree_serialize(RBTree *tree, const char *filename) {
    pthread_mutex_lock(&tree->lock);
    
    FILE *file = fopen(filename, "wb");
    if (!file) {
        pthread_mutex_unlock(&tree->lock);
        return false;
    }
    
    serialize_node(tree->root, file);
    fclose(file);
    
    log_operation(tree, "SERIALIZE", 0);
    pthread_mutex_unlock(&tree->lock);
    return true;
}

// Deserialization with validation
static Node* deserialize_node(FILE *file, Node *parent, bool *valid) {
    int key, color;
    
    if (fread(&key, sizeof(int), 1, file) != 1) {
        *valid = false;
        return NULL;
    }
    
    if (key == -1) return NULL;
    
    if (fread(&color, sizeof(int), 1, file) != 1 || (color != RED && color != BLACK)) {
        *valid = false;
        return NULL;
    }
    
    Node *node = (Node*)malloc(sizeof(Node));
    if (!node) {
        *valid = false;
        return NULL;
    }
    
    node->key = key;
    node->color = color;
    node->parent = parent;
    atomic_init(&node->ref_count, 1);
    
    bool left_valid = true, right_valid = true;
    node->left = deserialize_node(file, node, &left_valid);
    node->right = deserialize_node(file, node, &right_valid);
    
    *valid = left_valid && right_valid;
    
    // Validate Red-Black properties
    if (node->color == RED && parent && parent->color == RED) {
        *valid = false;
    }
    
    return node;
}

bool rb_tree_deserialize(RBTree *tree, const char *filename) {
    pthread_mutex_lock(&tree->lock);
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        pthread_mutex_unlock(&tree->lock);
        return false;
    }
    
    bool valid = true;
    Node *new_root = deserialize_node(file, NULL, &valid);
    fclose(file);
    
    if (!valid) {
        // Clean up partially constructed tree
        if (new_root) {
            decrement_ref(new_root);
        }
        pthread_mutex_unlock(&tree->lock);
        return false;
    }
    
    // Replace old tree with new one
    if (tree->root) {
        decrement_ref(tree->root);
    }
    tree->root = new_root;
    
    log_operation(tree, "DESERIALIZE", 0);
    pthread_mutex_unlock(&tree->lock);
    return true;
}

// Cleanup function
void rb_tree_destroy(RBTree *tree) {
    if (!tree) return;
    
    pthread_mutex_lock(&tree->lock);
    if (tree->root) {
        decrement_ref(tree->root);
    }
    
    fclose(tree->audit_log);
    pthread_mutex_destroy(&tree->lock);
    pthread_mutex_destroy(&tree->log_lock);
    free(tree);
}