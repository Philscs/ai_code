#include <stdio.h>
#include <stdlib.h>

// Define the structure for a node in the B+ tree
typedef struct Node {
    int key;
    int child[2]; // left and right child pointers
} BTreeNode;

// Function to create a new node
BTreeNode* newNode(int key) {
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    node->key = key;
    node->child[0] = NULL; // initial left child is NULL
    node->child[1] = NULL; // initial right child is NULL
    return node;
}

// Function to insert a new key into the B+ tree
void insert(BTreeNode* root, int key) {
    if (root == NULL) {
        root = newNode(key);
    } else {
        if (key < root->key) {
            // Insert key in left subtree
            if (root->child[0] == NULL) {
                root->child[0] = newNode(key);
            } else {
                insert(root->child[0], key);
            }
        } else if (key > root->key) {
            // Insert key in right subtree
            if (root->child[1] == NULL) {
                root->child[1] = newNode(key);
            } else {
                insert(root->child[1], key);
            }
        } else {
            // If key already exists, do nothing
        }
    }

    // Balance the tree
    balanceTree(root);
}

// Function to balance the B+ tree
void balanceTree(BTreeNode* root) {
    if (root == NULL || root->child[0] == NULL && root->child[1] == NULL) {
        return;
    }

    int height = getHeight(root);
    // Left subtree is too full, rotate right
    if (height > 1 && getHeight(root->child[0]) >= 2) {
        rotateRight(root);
    }
    // Right subtree is too full, rotate left
    else if (height > 1 && getHeight(root->child[1]) >= 2) {
        rotateLeft(root);
    }
}

// Function to get the height of a node
int getHeight(BTreeNode* root) {
    if (root == NULL) {
        return -1;
    } else {
        int leftHeight = getHeight(root->child[0]);
        int rightHeight = getHeight(root->child[1]);

        if (leftHeight > 0 && rightHeight > 0) {
            return leftHeight + 1;
        } else if (leftHeight > 0) {
            return leftHeight + 1;
        } else {
            return rightHeight + 1;
        }
    }
}

// Function to rotate a node
void rotateRight(BTreeNode* root) {
    BTreeNode* temp = root->child[0];
    root->child[0] = temp->child[1];
    if (temp->child[1] != NULL) {
        temp->child[1]->parent = temp;
    }

    temp->parent = root;
    temp->child[1] = root;

    if (root->child[1] == NULL) {
        root->child[1] = temp->child[0];
    }
}

void rotateLeft(BTreeNode* root) {
    BTreeNode* temp = root->child[1];
    root->child[1] = temp->child[0];
    if (temp->child[0] != NULL) {
        temp->child[0]->parent = temp;
    }

    temp->parent = root;
    temp->child[0] = root;

    if (root->child[0] == NULL) {
        root->child[0] = temp->child[1];
    }
}
