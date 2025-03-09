#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHAR 256
#define MAX_LENGTH 1000

// Struktur für einen Knoten im Suffix Tree
typedef struct SuffixTreeNode {
    struct SuffixTreeNode *children[MAX_CHAR];
    struct SuffixTreeNode *suffixLink;
    int start;
    int *end;
    int suffixIndex;
} Node;

// Globale Variablen für den Baum
char text[MAX_LENGTH];
Node *root = NULL;
Node *lastNewNode = NULL;
Node *activeNode = NULL;
int activeEdge = -1;
int activeLength = 0;
int remainingSuffixCount = 0;
int leafEnd = -1;
int *rootEnd = NULL;
int *splitEnd = NULL;
int size = -1;

// Funktion zum Erstellen eines neuen Knotens
Node* newNode(int start, int *end) {
    Node *node = (Node*)malloc(sizeof(Node));
    for (int i = 0; i < MAX_CHAR; i++)
        node->children[i] = NULL;
    
    node->suffixLink = root;
    node->start = start;
    node->end = end;
    node->suffixIndex = -1;
    return node;
}

// Initialisierung des Suffix Trees
void initTree() {
    rootEnd = (int*)malloc(sizeof(int));
    *rootEnd = -1;
    root = newNode(-1, rootEnd);
    activeNode = root;
    for (int i = 0; i < MAX_CHAR; i++)
        root->children[i] = NULL;
}

// Edge-Length Berechnung
int edgeLength(Node *n) {
    return *(n->end) - (n->start) + 1;
}

// Prüft, ob der aktuelle Knoten ein Blatt ist
int isLeaf(Node *n) {
    return n->suffixIndex != -1;
}

// Hauptfunktion zum Einfügen eines Suffixes
void extendSuffixTree(int pos) {
    leafEnd = pos;
    remainingSuffixCount++;
    lastNewNode = NULL;

    while (remainingSuffixCount > 0) {
        if (activeLength == 0)
            activeEdge = pos;

        if (activeNode->children[text[activeEdge]] == NULL) {
            activeNode->children[text[activeEdge]] = 
                newNode(pos, &leafEnd);
            if (lastNewNode != NULL) {
                lastNewNode->suffixLink = activeNode;
                lastNewNode = NULL;
            }
        } else {
            Node *next = activeNode->children[text[activeEdge]];
            if (walkDown(next))
                continue;

            if (text[next->start + activeLength] == text[pos]) {
                if (lastNewNode != NULL && activeNode != root) {
                    lastNewNode->suffixLink = activeNode;
                    lastNewNode = NULL;
                }
                activeLength++;
                break;
            }

            splitEnd = (int*)malloc(sizeof(int));
            *splitEnd = next->start + activeLength - 1;
            Node *split = newNode(next->start, splitEnd);
            activeNode->children[text[activeEdge]] = split;
            split->children[text[pos]] = newNode(pos, &leafEnd);
            next->start += activeLength;
            split->children[text[next->start]] = next;

            if (lastNewNode != NULL)
                lastNewNode->suffixLink = split;
            lastNewNode = split;
        }

        remainingSuffixCount--;
        if (activeNode == root && activeLength > 0) {
            activeLength--;
            activeEdge = pos - remainingSuffixCount + 1;
        } else if (activeNode != root) {
            activeNode = activeNode->suffixLink;
        }
    }
}

// Hilfsfunktion für das Traversieren des Baums
int walkDown(Node *currNode) {
    int len = edgeLength(currNode);
    if (activeLength >= len) {
        activeEdge += len;
        activeLength -= len;
        activeNode = currNode;
        return 1;
    }
    return 0;
}

// Pattern Matching Funktion
void searchPattern(char *pattern) {
    int patternLen = strlen(pattern);
    Node *current = root;
    int i = 0;
    
    while (i < patternLen) {
        int currentChar = pattern[i];
        if (current->children[currentChar] == NULL) {
            printf("Pattern nicht gefunden\n");
            return;
        }
        
        Node *next = current->children[currentChar];
        int j;
        for (j = 0; j < edgeLength(next) && i < patternLen; j++, i++) {
            if (text[next->start + j] != pattern[i]) {
                printf("Pattern nicht gefunden\n");
                return;
            }
        }
        if (i == patternLen) {
            printf("Pattern gefunden\n");
            return;
        }
        current = next;
    }
}

// Hauptfunktion zum Aufbau des Suffix Trees
void buildSuffixTree() {
    size = strlen(text);
    initTree();
    for (int i = 0; i < size; i++)
        extendSuffixTree(i);
}

// Beispiel Hauptprogramm
int main() {
    printf("Geben Sie den Text ein: ");
    scanf("%s", text);
    
    buildSuffixTree();
    
    char pattern[100];
    printf("Geben Sie das Suchmuster ein: ");
    scanf("%s", pattern);
    
    searchPattern(pattern);
    
    return 0;
}