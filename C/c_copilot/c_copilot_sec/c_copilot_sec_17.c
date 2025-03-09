#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct Node {
    int key;
    struct Node* left;
    struct Node* right;
    pthread_mutex_t lock;
} Node;

Node* createNode(int key) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    newNode->key = key;
    newNode->left = NULL;
    newNode->right = NULL;
    pthread_mutex_init(&newNode->lock, NULL);
    return newNode;
}

void destroyNode(Node* node) {
    pthread_mutex_destroy(&node->lock);
    free(node);
}

Node* insert(Node* root, int key) {
    if (root == NULL) {
        return createNode(key);
    }

    if (key < root->key) {
        pthread_mutex_lock(&root->lock);
        root->left = insert(root->left, key);
        pthread_mutex_unlock(&root->lock);
    } else if (key > root->key) {
        pthread_mutex_lock(&root->lock);
        root->right = insert(root->right, key);
        pthread_mutex_unlock(&root->lock);
    }

    return root;
}

Node* deleteNode(Node* root, int key) {
    if (root == NULL) {
        return root;
    }

    if (key < root->key) {
        pthread_mutex_lock(&root->lock);
        root->left = deleteNode(root->left, key);
        pthread_mutex_unlock(&root->lock);
    } else if (key > root->key) {
        pthread_mutex_lock(&root->lock);
        root->right = deleteNode(root->right, key);
        pthread_mutex_unlock(&root->lock);
    } else {
        if (root->left == NULL) {
            Node* temp = root->right;
            destroyNode(root);
            return temp;
        } else if (root->right == NULL) {
            Node* temp = root->left;
            destroyNode(root);
            return temp;
        }

        Node* temp = root->right;
        while (temp->left != NULL) {
            temp = temp->left;
        }

        pthread_mutex_lock(&temp->lock);
        root->key = temp->key;
        root->right = deleteNode(root->right, temp->key);
        pthread_mutex_unlock(&temp->lock);
    }

    return root;
}

void inorderTraversal(Node* root) {
    if (root != NULL) {
        inorderTraversal(root->left);
        printf("%d ", root->key);
        inorderTraversal(root->right);
    }
}

int main() {
    Node* root = NULL;

    root = insert(root, 50);
    root = insert(root, 30);
    root = insert(root, 20);
    root = insert(root, 40);
    root = insert(root, 70);
    root = insert(root, 60);
    root = insert(root, 80);

    printf("Inorder traversal of the BST: ");
    inorderTraversal(root);
    printf("\n");

    root = deleteNode(root, 20);
    root = deleteNode(root, 30);
    root = deleteNode(root, 50);

    printf("Inorder traversal after deletion: ");
    inorderTraversal(root);
    printf("\n");

    // Boundary Checking
    if (root != NULL) {
        destroyNode(root);
    }

    return 0;
}
