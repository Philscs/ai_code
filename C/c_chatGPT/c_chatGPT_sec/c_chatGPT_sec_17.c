#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

// Define the structure for a node in the AVL tree
typedef struct AVLNode {
    int key;
    int height;
    struct AVLNode *left, *right;
    pthread_mutex_t lock;
} AVLNode;

// Utility function to create a new AVLNode
AVLNode* createNode(int key) {
    AVLNode *node = (AVLNode *)malloc(sizeof(AVLNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->key = key;
    node->height = 1;
    node->left = node->right = NULL;
    pthread_mutex_init(&node->lock, NULL);
    return node;
}

// Utility function to get the height of a node
int height(AVLNode *node) {
    return node ? node->height : 0;
}

// Utility function to get the balance factor of a node
int getBalance(AVLNode *node) {
    return node ? height(node->left) - height(node->right) : 0;
}

// Right rotate subtree rooted with y
AVLNode* rightRotate(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = 1 + (height(y->left) > height(y->right) ? height(y->left) : height(y->right));
    x->height = 1 + (height(x->left) > height(x->right) ? height(x->left) : height(x->right));

    return x;
}

// Left rotate subtree rooted with x
AVLNode* leftRotate(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = 1 + (height(x->left) > height(x->right) ? height(x->left) : height(x->right));
    y->height = 1 + (height(y->left) > height(y->right) ? height(y->left) : height(y->right));

    return y;
}

// Insert a key into the AVL tree with thread safety
AVLNode* insert(AVLNode *node, int key) {
    if (!node)
        return createNode(key);

    pthread_mutex_lock(&node->lock);

    if (key < node->key)
        node->left = insert(node->left, key);
    else if (key > node->key)
        node->right = insert(node->right, key);
    else {
        pthread_mutex_unlock(&node->lock);
        return node; // Equal keys are not allowed
    }

    node->height = 1 + (height(node->left) > height(node->right) ? height(node->left) : height(node->right));

    int balance = getBalance(node);

    if (balance > 1 && key < node->left->key)
        node = rightRotate(node);

    if (balance < -1 && key > node->right->key)
        node = leftRotate(node);

    if (balance > 1 && key > node->left->key) {
        node->left = leftRotate(node->left);
        node = rightRotate(node);
    }

    if (balance < -1 && key < node->right->key) {
        node->right = rightRotate(node->right);
        node = leftRotate(node);
    }

    pthread_mutex_unlock(&node->lock);
    return node;
}

// Utility function to find the node with the smallest key
AVLNode* minValueNode(AVLNode* node) {
    AVLNode* current = node;
    while (current && current->left)
        current = current->left;
    return current;
}

// Delete a node from the AVL tree with thread safety
AVLNode* deleteNode(AVLNode* root, int key) {
    if (!root)
        return root;

    pthread_mutex_lock(&root->lock);

    if (key < root->key)
        root->left = deleteNode(root->left, key);
    else if (key > root->key)
        root->right = deleteNode(root->right, key);
    else {
        if (!root->left || !root->right) {
            AVLNode *temp = root->left ? root->left : root->right;

            pthread_mutex_unlock(&root->lock);
            pthread_mutex_destroy(&root->lock);
            free(root);

            return temp;
        }

        AVLNode* temp = minValueNode(root->right);
        root->key = temp->key;
        root->right = deleteNode(root->right, temp->key);
    }

    root->height = 1 + (height(root->left) > height(root->right) ? height(root->left) : height(root->right));

    int balance = getBalance(root);

    if (balance > 1 && getBalance(root->left) >= 0)
        root = rightRotate(root);

    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        root = rightRotate(root);
    }

    if (balance < -1 && getBalance(root->right) <= 0)
        root = leftRotate(root);

    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        root = leftRotate(root);
    }

    pthread_mutex_unlock(&root->lock);
    return root;
}

// Cleanup the entire tree
void cleanupTree(AVLNode* root) {
    if (!root)
        return;

    cleanupTree(root->left);
    cleanupTree(root->right);

    pthread_mutex_destroy(&root->lock);
    free(root);
}

// Main function
typedef struct {
    AVLNode *root;
    pthread_mutex_t tree_lock;
} AVLTree;

AVLTree* createTree() {
    AVLTree *tree = (AVLTree *)malloc(sizeof(AVLTree));
    if (!tree) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    tree->root = NULL;
    pthread_mutex_init(&tree->tree_lock, NULL);
    return tree;
}

void insertKey(AVLTree *tree, int key) {
    pthread_mutex_lock(&tree->tree_lock);
    tree->root = insert(tree->root, key);
    pthread_mutex_unlock(&tree->tree_lock);
}

void deleteKey(AVLTree *tree, int key) {
    pthread_mutex_lock(&tree->tree_lock);
    tree->root = deleteNode(tree->root, key);
    pthread_mutex_unlock(&tree->tree_lock);
}

void destroyTree(AVLTree *tree) {
    pthread_mutex_lock(&tree->tree_lock);
    cleanupTree(tree->root);
    pthread_mutex_unlock(&tree->tree_lock);
    pthread_mutex_destroy(&tree->tree_lock);
    free(tree);
}

int main() {
    AVLTree *tree = createTree();

    insertKey(tree, 10);
    insertKey(tree, 20);
    insertKey(tree, 30);
    insertKey(tree, 40);
    insertKey(tree, 50);

    deleteKey(tree, 20);

    destroyTree(tree);
    return 0;
}
