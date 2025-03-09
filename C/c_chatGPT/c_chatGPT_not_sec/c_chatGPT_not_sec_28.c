#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALPHABET_SIZE 256

// Struktur eines Suffixbaumknotens
typedef struct SuffixTreeNode {
    struct SuffixTreeNode *children[ALPHABET_SIZE];
    struct SuffixTreeNode *suffixLink;
    int start;
    int *end;
    int suffixIndex;
} SuffixTreeNode;

// Struktur für den Suffixbaum
typedef struct SuffixTree {
    char *text;
    SuffixTreeNode *root;
    SuffixTreeNode *lastNewNode;
    SuffixTreeNode *activeNode;
    int activeEdge;
    int activeLength;
    int remainingSuffixCount;
    int leafEnd;
    int *rootEnd;
    int *splitEnd;
    int size;
} SuffixTree;

// Hilfsfunktion zum Erstellen eines neuen Knotens
SuffixTreeNode *createNode(int start, int *end) {
    SuffixTreeNode *node = (SuffixTreeNode *)malloc(sizeof(SuffixTreeNode));
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        node->children[i] = NULL;
    }
    node->suffixLink = NULL;
    node->start = start;
    node->end = end;
    node->suffixIndex = -1;
    return node;
}

// Hilfsfunktion zur Erstellung eines Suffixbaums
SuffixTree *createSuffixTree(char *text) {
    SuffixTree *tree = (SuffixTree *)malloc(sizeof(SuffixTree));
    tree->size = strlen(text);
    tree->text = text;
    tree->rootEnd = (int *)malloc(sizeof(int));
    *(tree->rootEnd) = -1;
    tree->root = createNode(-1, tree->rootEnd);
    tree->activeNode = tree->root;
    tree->activeEdge = -1;
    tree->activeLength = 0;
    tree->lastNewNode = NULL;
    tree->remainingSuffixCount = 0;
    tree->leafEnd = -1;
    return tree;
}

// Erweiterungsfunktion für den Suffixbaum
void extendSuffixTree(SuffixTree *tree, int pos) {
    // Implementierung der Erweiterung des Suffixbaums
    // Platzhalter: Die vollständige Erweiterungslogik muss hinzugefügt werden
}

// Funktion, um einen Text im Suffixbaum zu suchen
int search(SuffixTree *tree, const char *pattern) {
    int patternLen = strlen(pattern);
    SuffixTreeNode *currentNode = tree->root;
    int index = 0;

    while (index < patternLen) {
        char currentChar = pattern[index];
        if (currentNode->children[currentChar] == NULL) {
            return 0; // Muster nicht gefunden
        }
        currentNode = currentNode->children[currentChar];
        int edgeLen = *(currentNode->end) - currentNode->start + 1;
        for (int i = 0; i < edgeLen && index < patternLen; i++, index++) {
            if (tree->text[currentNode->start + i] != pattern[index]) {
                return 0; // Muster nicht gefunden
            }
        }
    }

    return 1; // Muster gefunden
}

// Hauptprogramm
int main() {
    char text[] = "banana";
    char pattern[] = "nan";

    SuffixTree *tree = createSuffixTree(text);

    // Erstellen des Suffixbaums (vollständige Logik nicht implementiert)
    for (int i = 0; i < tree->size; i++) {
        extendSuffixTree(tree, i);
    }

    // Suche nach einem Muster
    if (search(tree, pattern)) {
        printf("Pattern '%s' found in the text '%s'.\n", pattern, text);
    } else {
        printf("Pattern '%s' not found in the text '%s'.\n", pattern, text);
    }

    return 0;
}
