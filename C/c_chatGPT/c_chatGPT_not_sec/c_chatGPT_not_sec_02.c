#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definition der Farben
#define RED 1
#define BLACK 0

// Struktur eines Knotens
typedef struct Node {
    int matrikelnummer;
    char name[50];
    float notendurchschnitt;
    int color; // 1 -> RED, 0 -> BLACK
    struct Node *left, *right, *parent;
} Node;

Node *root = NULL;
Node *TNULL;

// Hilfsfunktion zum Erstellen eines neuen Knotens
Node *createNode(int matrikelnummer, char *name, float notendurchschnitt) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->matrikelnummer = matrikelnummer;
    strcpy(node->name, name);
    node->notendurchschnitt = notendurchschnitt;
    node->color = RED;
    node->left = TNULL;
    node->right = TNULL;
    node->parent = NULL;
    return node;
}

// Initialisierung des TNULL-Knotens
void initializeTNULL() {
    TNULL = (Node *)malloc(sizeof(Node));
    TNULL->color = BLACK;
    TNULL->left = NULL;
    TNULL->right = NULL;
}

// Linksrotation
void leftRotate(Node *x) {
    Node *y = x->right;
    x->right = y->left;
    if (y->left != TNULL) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL) {
        root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

// Rechtsrotation
void rightRotate(Node *x) {
    Node *y = x->left;
    x->left = y->right;
    if (y->right != TNULL) {
        y->right->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL) {
        root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
}

// Reparatur nach dem Einfügen
void fixInsert(Node *k) {
    Node *u;
    while (k->parent->color == RED) {
        if (k->parent == k->parent->parent->right) {
            u = k->parent->parent->left; // Onkel
            if (u->color == RED) {
                u->color = BLACK;
                k->parent->color = BLACK;
                k->parent->parent->color = RED;
                k = k->parent->parent;
            } else {
                if (k == k->parent->left) {
                    k = k->parent;
                    rightRotate(k);
                }
                k->parent->color = BLACK;
                k->parent->parent->color = RED;
                leftRotate(k->parent->parent);
            }
        } else {
            u = k->parent->parent->right; // Onkel
            if (u->color == RED) {
                u->color = BLACK;
                k->parent->color = BLACK;
                k->parent->parent->color = RED;
                k = k->parent->parent;
            } else {
                if (k == k->parent->right) {
                    k = k->parent;
                    leftRotate(k);
                }
                k->parent->color = BLACK;
                k->parent->parent->color = RED;
                rightRotate(k->parent->parent);
            }
        }
        if (k == root) {
            break;
        }
    }
    root->color = BLACK;
}

// Einfügen eines neuen Knotens
void insert(int matrikelnummer, char *name, float notendurchschnitt) {
    Node *node = createNode(matrikelnummer, name, notendurchschnitt);
    Node *y = NULL;
    Node *x = root;

    while (x != TNULL) {
        y = x;
        if (node->matrikelnummer < x->matrikelnummer) {
            x = x->left;
        } else {
            x = x->right;
        }
    }

    node->parent = y;
    if (y == NULL) {
        root = node;
    } else if (node->matrikelnummer < y->matrikelnummer) {
        y->left = node;
    } else {
        y->right = node;
    }

    if (node->parent == NULL) {
        node->color = BLACK;
        return;
    }

    if (node->parent->parent == NULL) {
        return;
    }

    fixInsert(node);
}

// Inorder Traversierung (zum Anzeigen der Baumstruktur)
void inorder(Node *node) {
    if (node != TNULL) {
        inorder(node->left);
        printf("Matrikelnummer: %d, Name: %s, Notendurchschnitt: %.2f, Farbe: %s\n", 
            node->matrikelnummer, node->name, node->notendurchschnitt, 
            (node->color == RED) ? "Rot" : "Schwarz");
        inorder(node->right);
    }
}

int main() {
    initializeTNULL();
    root = TNULL;

    insert(101, "Alice", 1.7);
    insert(102, "Bob", 2.3);
    insert(103, "Charlie", 1.9);
    insert(104, "Diana", 2.1);
    insert(105, "Eve", 1.5);

    printf("Inorder Traversierung des Red-Black Trees:\n");
    inorder(root);

    return 0;
}
