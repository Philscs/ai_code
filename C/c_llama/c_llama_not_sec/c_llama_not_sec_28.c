#include <stdio.h>
#include <string.h>

// Strukt für einen Knoten im Suffixbaum
typedef struct {
    char c;  // Zeichen des Knotens
    int depth; // Tiefe des Knotens
} Node;

// Strukt für den Suffixbaum
typedef struct {
    Node* nodes[26]; // 0-25: Anzahl der Ziffern im Alphabet
    int end;        // Endknoten
} Tree;

// Funktion, um einen neuen Knoten im Baum zu erstellen
Node* create_node(char c) {
    Node* node = malloc(sizeof(Node));
    node->c = c;
    node->depth = 0;
    return node;
}

// Funktion, um den Suffixbaum zu erstellen
Tree* build_tree(const char* str) {
    Tree* tree = malloc(sizeof(Tree));
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (!tree->nodes[str[i] - 'a']) {
            tree->nodes[str[i] - 'a'] = create_node(str[i]);
        }
        tree->nodes[str[i] - 'a']->depth++;
        Node* node = tree->nodes[str[i] - 'a'];
        for (int j = i + 1; j < len; j++) {
            if (!tree->nodes[str[j] - 'a']) {
                break;
            }
            node = tree->nodes[str[j] - 'a'];
            node->depth++;
        }
    }
    tree->end = create_node('\0');
    return tree;
}

// Funktion, um nach einem bestimmten Muster zu suchen
int find_pattern(Tree* tree, const char* pattern) {
    for (int i = 0; i < 26; i++) {
        if (!tree->nodes[i]) {
            continue;
        }
        Node* node = tree->nodes[i];
        int len = strlen(pattern);
        int match_depth = -1;
        for (int j = 0; j < len; j++) {
            if (!node) {
                break;
            }
            char c = pattern[j] - 'a';
            if (c != i) {
                node = tree->nodes[c];
            } else {
                match_depth++;
                if (match_depth == node->depth) {
                    return 1; // Muster gefunden
                }
            }
        }
    }
    return 0; // Kein Muster gefunden
}

int main() {
    const char* str = "banana";
    Tree* tree = build_tree(str);
    const char* pattern = "ana";
    int result = find_pattern(tree, pattern);
    if (result) {
        printf("Muster gefunden!\n");
    } else {
        printf("Kein Muster gefunden.\n");
    }
    return 0;
}