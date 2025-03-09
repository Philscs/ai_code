#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RED 0
#define BLACK 1

// Struktur für einen Studenten
typedef struct {
    int matrikelnummer;
    char name[50];
    float notenschnitt;
} Student;

// Struktur für einen Baumknoten
typedef struct Node {
    Student data;
    int color;
    struct Node *left;
    struct Node *right;
    struct Node *parent;
} Node;

// Globale Variable für die Wurzel
Node *root = NULL;

// Hilfsfunktion: Erstellt einen neuen Knoten
Node* createNode(Student student) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = student;
    newNode->color = RED;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->parent = NULL;
    return newNode;
}

// Rotationen
void leftRotate(Node *x) {
    Node *y = x->right;
    x->right = y->left;
    
    if (y->left != NULL)
        y->left->parent = x;
    
    y->parent = x->parent;
    
    if (x->parent == NULL)
        root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    
    y->left = x;
    x->parent = y;
}

void rightRotate(Node *y) {
    Node *x = y->left;
    y->left = x->right;
    
    if (x->right != NULL)
        x->right->parent = y;
    
    x->parent = y->parent;
    
    if (y->parent == NULL)
        root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;
    
    x->right = y;
    y->parent = x;
}

// Balancierung nach dem Einfügen
void fixInsert(Node *k) {
    Node *u;
    while (k->parent != NULL && k->parent->color == RED) {
        if (k->parent == k->parent->parent->right) {
            u = k->parent->parent->left;
            if (u != NULL && u->color == RED) {
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
            u = k->parent->parent->right;
            if (u != NULL && u->color == RED) {
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
        if (k == root)
            break;
    }
    root->color = BLACK;
}

// Einfügen eines neuen Studenten
void insert(Student student) {
    Node* newNode = createNode(student);
    
    if (root == NULL) {
        root = newNode;
        root->color = BLACK;
        return;
    }
    
    Node *current = root;
    Node *parent = NULL;
    
    while (current != NULL) {
        parent = current;
        if (student.matrikelnummer < current->data.matrikelnummer)
            current = current->left;
        else
            current = current->right;
    }
    
    newNode->parent = parent;
    
    if (student.matrikelnummer < parent->data.matrikelnummer)
        parent->left = newNode;
    else
        parent->right = newNode;
    
    fixInsert(newNode);
}

// Suchen eines Studenten nach Matrikelnummer
Node* search(int matrikelnummer) {
    Node *current = root;
    
    while (current != NULL) {
        if (matrikelnummer == current->data.matrikelnummer)
            return current;
        else if (matrikelnummer < current->data.matrikelnummer)
            current = current->left;
        else
            current = current->right;
    }
    
    return NULL;
}

// Inorder-Traversierung zum Ausgeben aller Studenten
void inorderTraversal(Node *node) {
    if (node != NULL) {
        inorderTraversal(node->left);
        printf("Matrikelnummer: %d, Name: %s, Notenschnitt: %.2f, Farbe: %s\n",
               node->data.matrikelnummer,
               node->data.name,
               node->data.notenschnitt,
               node->color == RED ? "Rot" : "Schwarz");
        inorderTraversal(node->right);
    }
}

// Hauptfunktion zum Testen
int main() {
    // Beispielstudenten
    Student s1 = {12345, "Max Mustermann", 2.3};
    Student s2 = {67890, "Lisa Mueller", 1.7};
    Student s3 = {11111, "Tom Schmidt", 2.0};
    
    // Einfügen der Studenten
    insert(s1);
    insert(s2);
    insert(s3);
    
    // Ausgabe aller Studenten
    printf("Alle Studenten (inorder):\n");
    inorderTraversal(root);
    
    // Suche nach einem Studenten
    int suchMatrikel = 67890;
    Node* gefunden = search(suchMatrikel);
    if (gefunden != NULL) {
        printf("\nGefundener Student:\n");
        printf("Name: %s, Notenschnitt: %.2f\n",
               gefunden->data.name,
               gefunden->data.notenschnitt);
    } else {
        printf("\nStudent mit Matrikelnummer %d nicht gefunden.\n", suchMatrikel);
    }
    
    return 0;
}