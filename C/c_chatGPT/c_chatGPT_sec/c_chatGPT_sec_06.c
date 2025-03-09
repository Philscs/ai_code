#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <json-c/json.h>

// Red-Black Tree Node Colors
typedef enum { RED, BLACK } Color;

// Node structure for Red-Black Tree
typedef struct Node {
    int key;
    Color color;
    struct Node *left, *right, *parent;
} Node;

// Red-Black Tree structure
typedef struct RBTree {
    Node *root;
    Node *nil;
    pthread_mutex_t lock;
} RBTree;

// Audit Logging Function
void log_operation(const char *operation, int key) {
    FILE *log_file = fopen("rbtree_audit.log", "a");
    if (log_file) {
        fprintf(log_file, "Operation: %s, Key: %d\n", operation, key);
        fclose(log_file);
    }
}

// Utility Function: Create a new node
Node *create_node(RBTree *tree, int key) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->key = key;
    node->color = RED;
    node->left = tree->nil;
    node->right = tree->nil;
    node->parent = tree->nil;
    return node;
}

// Initialize Red-Black Tree
RBTree *initialize_rbtree() {
    RBTree *tree = (RBTree *)malloc(sizeof(RBTree));
    if (!tree) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    tree->nil = (Node *)malloc(sizeof(Node));
    if (!tree->nil) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    tree->nil->color = BLACK;
    tree->root = tree->nil;
    pthread_mutex_init(&tree->lock, NULL);
    return tree;
}

// Left Rotate Function
void left_rotate(RBTree *tree, Node *x) {
    Node *y = x->right;
    x->right = y->left;
    if (y->left != tree->nil) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == tree->nil) {
        tree->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

// Right Rotate Function
void right_rotate(RBTree *tree, Node *y) {
    Node *x = y->left;
    y->left = x->right;
    if (x->right != tree->nil) {
        x->right->parent = y;
    }
    x->parent = y->parent;
    if (y->parent == tree->nil) {
        tree->root = x;
    } else if (y == y->parent->right) {
        y->parent->right = x;
    } else {
        y->parent->left = x;
    }
    x->right = y;
    y->parent = x;
}

// Insert Fixup Function
void insert_fixup(RBTree *tree, Node *z) {
    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            Node *y = z->parent->parent->right;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate(tree, z->parent->parent);
            }
        } else {
            Node *y = z->parent->parent->left;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

// Insert Function
void insert(RBTree *tree, int key) {
    pthread_mutex_lock(&tree->lock);

    Node *z = create_node(tree, key);
    Node *y = tree->nil;
    Node *x = tree->root;

    while (x != tree->nil) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        } else {
            x = x->right;
        }
    }
    z->parent = y;
    if (y == tree->nil) {
        tree->root = z;
    } else if (z->key < y->key) {
        y->left = z;
    } else {
        y->right = z;
    }
    z->left = tree->nil;
    z->right = tree->nil;
    z->color = RED;

    insert_fixup(tree, z);

    log_operation("Insert", key);
    pthread_mutex_unlock(&tree->lock);
}

// Serialize Red-Black Tree to JSON
void serialize_node(Node *node, Node *nil, struct json_object *jobj) {
    if (node == nil) return;

    struct json_object *node_obj = json_object_new_object();
    json_object_object_add(node_obj, "key", json_object_new_int(node->key));
    json_object_object_add(node_obj, "color", json_object_new_string(node->color == RED ? "RED" : "BLACK"));

    struct json_object *left_obj = json_object_new_object();
    serialize_node(node->left, nil, left_obj);
    json_object_object_add(node_obj, "left", left_obj);

    struct json_object *right_obj = json_object_new_object();
    serialize_node(node->right, nil, right_obj);
    json_object_object_add(node_obj, "right", right_obj);

    json_object_object_add(jobj, "node", node_obj);
}

void serialize_tree(RBTree *tree) {
    pthread_mutex_lock(&tree->lock);

    struct json_object *jobj = json_object_new_object();
    serialize_node(tree->root, tree->nil, jobj);

    const char *json_str = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);
    FILE *file = fopen("rbtree.json", "w");
    if (file) {
        fprintf(file, "%s\n", json_str);
        fclose(file);
    }
    json_object_put(jobj);

    pthread_mutex_unlock(&tree->lock);
}

// Main Function for Testing
int main() {
    RBTree *tree = initialize_rbtree();

    insert(tree, 10);
    insert(tree, 20);
    insert(tree, 30);
    insert(tree, 40);
    insert(tree, 50);

    serialize_tree(tree);

    return 0;
}
