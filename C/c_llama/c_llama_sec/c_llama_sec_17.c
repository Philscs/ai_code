#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Struktdefinition für ein AVL-Node
typedef struct Node {
    int key;
    struct Node* left;
    struct Node* right;
    int height;
} Node;

// Struktdefinition für einen AVL-BST
typedef struct BST {
    Node* root;
} BST;

// Funktion zur Erstellung eines neuen Nodes
Node* create_node(int key) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (!node) {
        printf("Memory Error\n");
        exit(1);
    }
    node->key = key;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    return node;
}

// Funktion zur Erstellung eines neuen AVL-BST
BST* create_bst() {
    BST* bst = (BST*)malloc(sizeof(BST));
    if (!bst) {
        printf("Memory Error\n");
        exit(1);
    }
    bst->root = NULL;
    return bst;
}

// Funktion zum Erhöhen des Node-Heights
int update_height(Node* node) {
    if (!node) return 0;
    int left_height, right_height;
    left_height = (node->left ? node->left->height : 0);
    right_height = (node->right ? node->right->height : 0);

    node->height = 1 + (left_height > right_height ? left_height : right_height);
    return node->height;
}

// Funktion zum Auswerten des AVL-Balancemodus
int is_balanced(Node* node) {
    if (!node) return 1;
    int left_height, right_height;
    left_height = (node->left ? node->left->height : 0);
    right_height = (node->right ? node->right->height : 0);

    return abs(left_height - right_height) <= 1 && is_balanced(node->left) && 
is_balanced(node->right);
}

// Funktion zum Rotation des linken Nodes
Node* left_rotate(Node* node) {
    Node* temp = node->right;
    node->right = temp->left;
    temp->left = node;

    update_height(node);
    update_height(temp);

    return temp;
}

// Funktion zum Rotation des rechten Nodes
Node* right_rotate(Node* node) {
    Node* temp = node->left;
    node->left = temp->right;
    temp->right = node;

    update_height(node);
    update_height(temp);

    return temp;
}

// Funktion zum Umstellen eines Node im AVL-BST
Node* balance_node(Node* node) {
    int left_height, right_height;
    left_height = (node->left ? node->left->height : 0);
    right_height = (node->right ? node->right->height : 0);

    if (left_height > right_height + 1)
        return right_rotate(node);
    else if (right_height > left_height + 1)
        return left_rotate(node);
    else
        return node;
}

// Funktion zum Hinzufügen eines Neuen Knotens im AVL-BST
Node* insert_node(Node* node, int key) {
    if (!node) {
        Node* new_node = create_node(key);
        return new_node;
    }

    if (key < node->key)
        node->left = insert_node(node->left, key);
    else if (key > node->key)
        node->right = insert_node(node->right, key);
    else
        printf("Duplicate Key\n");

    node = balance_node(node);

    return node;
}

// Funktion zum Löschen eines Knotens im AVL-BST
Node* delete_node(Node* node, int key) {
    if (!node) return NULL;

    if (key < node->key)
        node->left = delete_node(node->left, key);
    else if (key > node->key)
        node->right = delete_node(node->right, key);
    else
    {
        if (!node->left && !node->right)
            free(node);
        else
        {
            Node* temp = (node->left ? node->left : node->right);
            free(node);
            return temp;
        }
    }

    node = balance_node(node);

    return node;
}

// Funktion zum Suchen eines Wertes im AVL-BST
Node* search_node(Node* node, int key) {
    if (!node || node->key == key)
        return node;

    if (key < node->key)
        return search_node(node->left, key);
    else
        return search_node(node->right, key);
}

// Funktion zum Überprüfen der Existenz eines Wertes im AVL-BST
int exists_node(BST* bst, int key) {
    Node* temp = search_node(bst->root, key);

    if (!temp)
        printf("Key %d does not exist in the BST.\n", key);
    else
        return 1;
}

// Funktion zum Erweitern des AVL-BSTs
void expand_bst(BST* bst) {
    if (bst->root == NULL)
        printf("The tree is empty.\n");

    Node* node = bst->root;
    while (node && node->left)
    {
        Node* temp = node->left;
        node->right = temp->right;

        while (temp->right)
            temp = temp->right;

        if (!temp->right)
            temp->right = create_node(0);

        node = balance_node(node);
    }

    bst->root = NULL;
}

// Funktion zum Konkatenieren zweier AVL-BSTs
BST* concat_bst(BST* bst1, BST* bst2) {
    if (!bst1 || !bst2)
        return (bst1 ? bst1 : bst2);

    Node* temp = create_node(0);
    temp->left = bst1->root;
    temp->right = bst2->root;

    bst1->root = bst2->root = NULL;

    return concat_bst(temp, NULL);
}

int main() {
    // Erstellung eines neuen AVL-BSTs
    BST* bst = create_bst();

    // Erster Knoten hinzufügen
    insert_node(bst->root, 10);

    // Zweiter Knoten hinzufügen
    insert_node(bst->root, 20);

    // Dritter Knoten hinzufügen
    insert_node(bst->root, 30);

    // Vierter Knoten hinzufügen
    insert_node(bst->root, 40);

    // Fifth Knoten hinzufügen
    insert_node(bst->root, 50);

    // Sechster Knoten hinzufügen
    insert_node(bst->root, 60);

    printf("Key 10 exists in the BST: %s\n", exists_node(bst, 10) ? "true" : "false");
    printf("Key 70 exists in the BST: %s\n", exists_node(bst, 70) ? "true" : "false");

    // Suchen von Knoten
    Node* temp = search_node(bst->root, 30);
    if (temp)
        printf("Found node with key 30.\n");
    else
        printf("No such node exists in the BST.\n");

    // Löschen eines Knotens
    delete_node(bst->root, 20);

    // Erweitern des AVL-BSTs
    expand_bst(bst);

    return 0;
}