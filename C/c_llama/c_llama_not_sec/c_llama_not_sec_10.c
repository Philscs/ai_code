#include <stdio.h>
#include <stdlib.h>

#define T 3 // Mindest Zielkind

// Typ definition für einen Knoten im B-Tree
typedef struct Node {
    int key; // Schlüssel des Knotens
    int n; // Anzahl der Kinder im Knoten
    struct Node** children; // Array von Kindern (Punktern auf `children`)
    struct Node* parent; // Elterner Knoten (NULL für Wurzel)
} BTreeNode;

// Typ definition für die Datenbank-Engine
typedef struct Database {
    BTreeNode* root; // Wurzel des B-Trees
    int txid; // TransaktionsID
} Database;

// Funktion zum Erstellen eines neuen Knotens im B-Tree
BTreeNode* createNode(int key) {
    BTreeNode* node = malloc(sizeof(BTreeNode));
    node->key = key;
    node->n = 0;
    node->children = NULL;
    node->parent = NULL;
    return node;
}

// Funktion zum Hinzufügen eines Schlüssels zu einem Knoten
void addNode(BTreeNode* node, int key) {
    // Finden des richtigen Platzes für den Schlüssel
    for (int i = 0; i < node->n; i++) {
        if (key <= node->children[i]->key) break;
    }

    // Hinzufügen des Schlüssels
    BTreeNode* newNode = createNode(key);
    newNode->parent = node;
    newNode->children = &node->children[node->n];
    node->children[node->n] = newNode;

    // Erhöhen der Anzahl der Kinder im Knoten
    node->n++;
}

// Funktion zum Auslesen eines Schlüssels aus einem Knoten
int readNode(BTreeNode* node, int i) {
    if (node == NULL || i < 0 || i >= node->n) return -1;
    return node->children[i]->key;
}

// Funktion zur Aktualisierung der Anzahl der Kinder in einem Knoten
void updateNode(BTreeNode* node) {
    for (int i = 0; i < node->n; i++) {
        BTreeNode* child = node->children[i];
        child->parent = node;
        updateNode(child);
    }
}

// Funktion zum Erstellen des Wurzels des B-Trees
BTreeNode* createRoot() {
    return createNode(-1);
}

// Funktion zum Speichern eines Knotens im B-Tree
void saveNode(BTreeNode* node, FILE* file) {
    fprintf(file, "%d ", node->key);
    for (int i = 0; i < node->n; i++) {
        saveNode(node->children[i], file);
    }
}

// Funktion zum Lösen eines Knotens im B-Tree
void deleteNode(BTreeNode* node) {
    if (node == NULL || node->n == 0) return;

    // Finden des Nachfolgers
    int i;
    for (i = 0; i < node->n; i++) {
        BTreeNode* child = node->children[i];
        if (child->key != -1 && child->parent == NULL) break;
    }

    // Löschen des Knotens
    if (node->n > T) {
        // Vervollständigen des Knotens
        for (i = 0; i < node->n; i++) {
            BTreeNode* child = node->children[i];
            deleteNode(child);
        }
        addNode(node, -1);
    } else if (node->n > 1) {
        // Lösen des Nachfolgers
        for (i = 0; i < node->n; i++) {
            BTreeNode* child = node->children[i];
            if (child->key != -1 && child->parent == NULL) break;
        }

        // Ersetzen des Knotens durch den Nachfolger
        int j = i + 1;
        for (; j < node->n; j++) {
            BTreeNode* child = node->children[j];
            if (child->key != -1 && child->parent == NULL) break;
        }

        // Löschen des Knotens und Ersetzen durch den Nachfolger
        deleteNode(node);
        addNode(node, child->key);
    } else {
        // Lösen des Knotens und Rücksetzen der Wurzel
        int i = 0;
        for (; i < node->n; i++) {
            BTreeNode* child = node->children[i];
            if (child->key != -1 && child->parent == NULL) break;
        }

        // Löschen des Knotens und Rücksetzen der Wurzel
        deleteNode(node);
        node->parent = NULL;
    }
}

// Funktion zum Erstellen einer Datenbank-Engine
Database createDB() {
    Database db = {NULL, 0};
    return db;
}

int main() {
    // Erstellung eines neuen Knotens
    BTreeNode* node = createNode(10);

    // Speicherung des Knotens im Datei
    FILE* file = fopen("example.dat", "w");
    saveNode(node, file);
    fclose(file);

    // Erstellung einer Datenbank-Engine
    Database db = createDB();

    // Aktivierung der Transaktion
    int txid = 1;
    db.txid = txid;

    // Erstellung des Wurzels des B-Trees
    BTreeNode* root = createRoot();
    db.root = root;

    // Auslesen eines Schlüssels aus dem Knoten
    printf("%d\n", readNode(root, 0));

    return 0;
}