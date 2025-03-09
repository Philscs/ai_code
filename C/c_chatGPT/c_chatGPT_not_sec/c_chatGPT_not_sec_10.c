#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define ORDER 3  // B-Tree Order (max keys per node = ORDER - 1)

typedef struct BTreeNode {
    int keys[ORDER - 1];
    struct BTreeNode *children[ORDER];
    bool isLeaf;
    int numKeys;
} BTreeNode;

typedef struct TransactionLog {
    char action[20];
    int key;
} TransactionLog;

BTreeNode *createNode(bool isLeaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    node->isLeaf = isLeaf;
    node->numKeys = 0;
    for (int i = 0; i < ORDER; i++) {
        node->children[i] = NULL;
    }
    return node;
}

void splitChild(BTreeNode *parent, int i, BTreeNode *child) {
    BTreeNode *newChild = createNode(child->isLeaf);
    newChild->numKeys = ORDER / 2 - 1;

    for (int j = 0; j < ORDER / 2 - 1; j++) {
        newChild->keys[j] = child->keys[j + ORDER / 2];
    }
    if (!child->isLeaf) {
        for (int j = 0; j < ORDER / 2; j++) {
            newChild->children[j] = child->children[j + ORDER / 2];
        }
    }
    child->numKeys = ORDER / 2 - 1;

    for (int j = parent->numKeys; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[i + 1] = newChild;

    for (int j = parent->numKeys - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
    }
    parent->keys[i] = child->keys[ORDER / 2 - 1];
    parent->numKeys++;
}

void insertNonFull(BTreeNode *node, int key) {
    int i = node->numKeys - 1;

    if (node->isLeaf) {
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->numKeys++;
    } else {
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        if (node->children[i]->numKeys == ORDER - 1) {
            splitChild(node, i, node->children[i]);
            if (key > node->keys[i]) {
                i++;
            }
        }
        insertNonFull(node->children[i], key);
    }
}

void insert(BTreeNode **root, int key) {
    if ((*root)->numKeys == ORDER - 1) {
        BTreeNode *newRoot = createNode(false);
        newRoot->children[0] = *root;
        splitChild(newRoot, 0, *root);
        int i = (key > newRoot->keys[0]) ? 1 : 0;
        insertNonFull(newRoot->children[i], key);
        *root = newRoot;
    } else {
        insertNonFull(*root, key);
    }
}

bool search(BTreeNode *node, int key) {
    int i = 0;
    while (i < node->numKeys && key > node->keys[i]) {
        i++;
    }
    if (i < node->numKeys && key == node->keys[i]) {
        return true;
    }
    if (node->isLeaf) {
        return false;
    }
    return search(node->children[i], key);
}

void logTransaction(TransactionLog *log, const char *action, int key) {
    strncpy(log->action, action, sizeof(log->action) - 1);
    log->key = key;
    printf("Transaction Logged: %s %d\n", action, key);
}

void rollbackTransaction(TransactionLog *log, BTreeNode **root) {
    if (strcmp(log->action, "INSERT") == 0) {
        // Implement rollback logic for insertion (e.g., delete the inserted key)
        printf("Rollback: DELETE %d\n", log->key);
    }
    // Other rollback cases (e.g., for DELETE) can be implemented similarly.
}

int main() {
    BTreeNode *root = createNode(true);
    TransactionLog log;

    insert(&root, 10);
    logTransaction(&log, "INSERT", 10);

    insert(&root, 20);
    logTransaction(&log, "INSERT", 20);

    insert(&root, 5);
    logTransaction(&log, "INSERT", 5);

    printf("Search 10: %s\n", search(root, 10) ? "Found" : "Not Found");
    printf("Search 15: %s\n", search(root, 15) ? "Found" : "Not Found");

    rollbackTransaction(&log, &root);

    return 0;
}