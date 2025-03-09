#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure to represent a node in the suffix tree
typedef struct Node {
    struct Node* children[256];
    struct Node* suffixLink;
    int start;
    int* end;
    int suffixIndex;
} Node;

// Function to create a new node
Node* createNode(int start, int* end) {
    Node* node = (Node*)malloc(sizeof(Node));
    for (int i = 0; i < 256; i++) {
        node->children[i] = NULL;
    }
    node->suffixLink = NULL;
    node->start = start;
    node->end = end;
    node->suffixIndex = -1;
    return node;
}

// Function to extend the suffix tree with a new suffix
void extendSuffixTree(Node** root, int* remainingSuffixCount, Node** lastNewNode, int* activeEdge, int* activeLength, int* remainingSuffix) {
    (*remainingSuffixCount)++;
    (*lastNewNode) = NULL;

    while ((*remainingSuffixCount) > 0) {
        if ((*activeLength) == 0) {
            (*activeEdge) = (*remainingSuffix);
        }

        if ((*root)->children[(unsigned char)(*activeEdge)] == NULL) {
            (*root)->children[(unsigned char)(*activeEdge)] = createNode((*remainingSuffix), remainingSuffix);
            if ((*lastNewNode) != NULL) {
                (*lastNewNode)->suffixLink = (*root);
                (*lastNewNode) = NULL;
            }
        } else {
            Node* next = (*root)->children[(unsigned char)(*activeEdge)];
            if (walkDown(next, activeEdge, activeLength, remainingSuffix)) {
                continue;
            }

            if (next->start + (*activeLength) - 1 == (*remainingSuffix)) {
                if ((*lastNewNode) != NULL && (*root) != (*lastNewNode)) {
                    (*lastNewNode)->suffixLink = (*root);
                    (*lastNewNode) = NULL;
                }
                (*activeLength)++;
                break;
            }

            int* splitEnd = (int*)malloc(sizeof(int));
            *splitEnd = next->start + (*activeLength) - 1;

            Node* split = createNode(next->start, splitEnd);
            (*root)->children[(unsigned char)(*activeEdge)] = split;

            split->children[(unsigned char)(*remainingSuffix)] = createNode((*remainingSuffix), remainingSuffix);
            next->start += (*activeLength);
            split->children[(unsigned char)(next->start)] = next;

            if ((*lastNewNode) != NULL) {
                (*lastNewNode)->suffixLink = split;
            }

            (*lastNewNode) = split;
        }

        (*remainingSuffixCount)--;
        if ((*root) == (*lastNewNode) && (*activeLength) > 0) {
            (*activeLength)--;
            (*activeEdge) = (*remainingSuffix) - (*remainingSuffixCount) + 1;
        } else if ((*root) != (*lastNewNode)) {
            (*root) = (*root)->suffixLink;
        }
    }
}

// Function to traverse the suffix tree and print all occurrences of a pattern
void traverseSuffixTree(Node* root, char* text, char* pattern) {
    if (root == NULL) {
        return;
    }

    int patternLength = strlen(pattern);
    if (root->suffixIndex != -1) {
        int start = root->suffixIndex - patternLength + 1;
        printf("Pattern found at index: %d\n", start);
    }

    for (int i = 0; i < 256; i++) {
        if (root->children[i] != NULL) {
            int edgeLength = *(root->children[i]->end) - root->children[i]->start + 1;
            char* edge = text + root->children[i]->start;
            if (strncmp(pattern, edge, edgeLength) == 0) {
                traverseSuffixTree(root->children[i], text, pattern);
            }
        }
    }
}

// Function to build the suffix tree
void buildSuffixTree(char* text, char* pattern) {
    int textLength = strlen(text);
    int patternLength = strlen(pattern);

    int remainingSuffixCount = 0;
    Node* root = createNode(-1, (int*)malloc(sizeof(int)));
    Node* lastNewNode = NULL;
    int activeEdge = -1;
    int activeLength = 0;
    int remainingSuffix = 0;

    for (int i = 0; i < textLength; i++) {
        remainingSuffixCount++;
        lastNewNode = NULL;

        while (remainingSuffixCount > 0) {
            if (activeLength == 0) {
                activeEdge = i;
            }

            if (root->children[(unsigned char)text[activeEdge]] == NULL) {
                root->children[(unsigned char)text[activeEdge]] = createNode(i, &remainingSuffix);
                if (lastNewNode != NULL) {
                    lastNewNode->suffixLink = root;
                    lastNewNode = NULL;
                }
            } else {
                Node* next = root->children[(unsigned char)text[activeEdge]];
                if (walkDown(next, &activeEdge, &activeLength, &remainingSuffix)) {
                    continue;
                }

                if (text[next->start + activeLength] == text[i]) {
                    if (lastNewNode != NULL && root != lastNewNode) {
                        lastNewNode->suffixLink = root;
                        lastNewNode = NULL;
                    }
                    activeLength++;
                    break;
                }

                int* splitEnd = (int*)malloc(sizeof(int));
                *splitEnd = next->start + activeLength - 1;

                Node* split = createNode(next->start, splitEnd);
                root->children[(unsigned char)text[activeEdge]] = split;

                split->children[(unsigned char)text[i]] = createNode(i, &remainingSuffix);
                next->start += activeLength;
                split->children[(unsigned char)text[next->start]] = next;

                if (lastNewNode != NULL) {
                    lastNewNode->suffixLink = split;
                }

                lastNewNode = split;
            }

            remainingSuffixCount--;
            if (root == lastNewNode && activeLength > 0) {
                activeLength--;
                activeEdge = i - remainingSuffixCount + 1;
            } else if (root != lastNewNode) {
                root = root->suffixLink;
            }
        }
    }

    traverseSuffixTree(root, text, pattern);
}

int main() {
    char text[] = "banana";
    char pattern[] = "ana";
    buildSuffixTree(text, pattern);
    return 0;
}
