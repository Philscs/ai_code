#include <stdio.h>
#include <stdlib.h>

// Define the structure for key-value pairs
typedef struct {
    int key;
    int value;
} KeyValuePair;

// Define the structure for a node in the tree
typedef struct {
    int numKeys;
    KeyValuePair* keys;
} Node;

// Function to create a new node
Node* createNode(int numKeys) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->numKeys = numKeys;
    newNode->keys = (KeyValuePair*)malloc(numKeys * sizeof(KeyValuePair));
    return newNode;
}

// Function to insert a key-value pair into the tree
void insert(Node* root, int key, int value) {
    // Find the appropriate position to insert the key-value pair
    int i = root->numKeys - 1;
    while (i >= 0 && root->keys[i].key > key) {
        root->keys[i + 1] = root->keys[i];
        i--;
    }
    root->keys[i + 1].key = key;
    root->keys[i + 1].value = value;
}

// Function to search for a key in the tree
int search(Node* root, int key) {
    int i = 0;
    while (i < root->numKeys && root->keys[i].key < key) {
        i++;
    }
    if (i < root->numKeys && root->keys[i].key == key) {
        return root->keys[i].value;
    }
    return -1;
}

int main() {
    // Create a root node
    Node* root = createNode(3);

    // Insert key-value pairs into the tree
    insert(root, 5, 10);
    insert(root, 3, 20);
    insert(root, 8, 30);

    // Search for a key in the tree
    int value = search(root, 3);
    if (value != -1) {
        printf("Value found: %d\n", value);
    } else {
        printf("Value not found\n");
    }

    // Free the memory allocated for the tree
    free(root->keys);
    free(root);

    return 0;
}
