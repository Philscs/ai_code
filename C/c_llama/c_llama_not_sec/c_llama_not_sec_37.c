#include <stdio.h>
#include <stdlib.h>

// Struktdefinition für den Knoten im LSMT
typedef struct Node {
    int key;
    char value[256]; // Wert des Schlüssels
    int leftChild, rightChild;
} Node;

// Struktdefinition für den LSMT
typedef struct {
    int numKeys; // Anzahl der Schlüssel im Baum
    Node* root; // Wurzel des Baus
} LSMTree;

// Funktion zur Erstellung eines neuen Knotens
Node* newNode(int key, char value[]) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->key = key;
    strcpy(node->value, value);
    node->leftChild = -1;
    node->rightChild = -1;
    return node;
}

// Funktion zur Erstellung eines neuen LSMT
LSMTree* createLSMTree() {
    LSMTree* lsmTree = (LSMTree*)malloc(sizeof(LSMTree));
    lsmTree->numKeys = 0;
    lsmTree->root = NULL;
    return lsmTree;
}

// Funktion zur Einfügung eines neuen Schlüssel-Wert-Paars
void insertLSMTree(LSMTree* lsmTree, int key, char value[]) {
    if (lsmTree->numKeys == 0) {
        lsmTree->root = newNode(key, value);
        lsmTree->numKeys++;
        return;
    }

    // Suche nach dem Knoten, der das gewünschte Schlüssel-Wert-Paar enthalten soll
    Node* currentNode = lsmTree->root;
    while (currentNode != NULL) {
        if (currentNode->key < key) {
            currentNode = currentNode->rightChild;
        } else if (currentNode->key > key) {
            currentNode = currentNode->leftChild;
        } else {
            // Wenn das gewünschte Schlüssel-Wert-Paar bereits existiert, füge es zu dem Knoten 
hinzu
            strcat(currentNode->value, value);
            return;
        }
    }

    // Wenn das gewünschte Schlüssel-Wert-Paar nicht gefunden wurde, füge es zur Wurzel des Baus 
hinzu
    Node* newNode = newNode(key, value);
    if (lsmTree->root == NULL) {
        lsmTree->root = newNode;
    } else {
        // Suche nach dem Knoten, der das gewünschte Schlüssel-Wert-Paar enthalten soll und füge 
es zu diesem Knoten hinzu
        Node* currentNode = lsmTree->root;
        while (currentNode != NULL) {
            if (currentNode->key < key) {
                newNode->leftChild = currentNode->rightChild;
                currentNode->rightChild = newNode;
                break;
            }
            currentNode = currentNode->rightChild;
        }
    }

    // Aktualisiere die Anzahl der Schlüssel im Baum
    lsmTree->numKeys++;
}

// Funktion zur Abfrage eines Schlüssel-Wert-Paars
char* queryLSMTree(LSMTree* lsmTree, int key) {
    // Suche nach dem Knoten, der das gewünschte Schlüssel-Wert-Paar enthalten soll
    Node* currentNode = lsmTree->root;
    while (currentNode != NULL) {
        if (currentNode->key == key) {
            return currentNode->value; // Rückgabewert des Wertes
        }
        if (currentNode->key < key) {
            currentNode = currentNode->rightChild;
        } else {
            currentNode = currentNode->leftChild;
        }
    }

    // Wenn das gewünschte Schlüssel-Wert-Paar nicht gefunden wurde, returnt NULL
    return NULL;
}

// Funktion zur Aufräumung des LSMTs
void freeLSMTree(LSMTree* lsmTree) {
    if (lsmTree->root == NULL) {
        return;
    }

    // Rekurze Aufräumung der linke Hälfte des Baus
    freeLSMTreeNode(lsmTree->root);

    // Wiederholen der Funktion für die rechte Hälfte des Baus
    freeLSMTree(lsmTree);
}

// Funktion zur Aufräumung eines Knotens im LSMT
void freeLSMTreeNode(Node* node) {
    if (node == NULL) {
        return;
    }

    // Rekurze Aufräumung der linken Hälfte des Knotens
    freeLSMTreeNode(node->leftChild);

    // Rekurse Aufräumung der rechten Hälfte des Knotens
    freeLSMTreeNode(node->rightChild);

    // Freigabe der Speicherressourcen des Knotens
    free(node);
}

int main() {
    LSMTree* lsmTree = createLSMTree();

    // Einfügung eines neuen Schlüssel-Wert-Paars
    insertLSMTree(lsmTree, 1, "Wert 1");
    insertLSMTree(lsmTree, 2, "Wert 2");

    // Abfrage eines Schlüssel-Wert-Paars
    char* queryValue = queryLSMTree(lsmTree, 1);
    printf("Query Value: %s\n", queryValue);

    // Aufräumung des LSMTs
    freeLSMTree(lsmTree);

    return 0;
}
