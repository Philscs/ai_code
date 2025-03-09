#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <stdbool.h>

// Struktur für einen Baumknoten
typedef struct Node {
    int key;
    struct Node *left;
    struct Node *right;
    int height;
    pthread_mutex_t lock;  // Mutex für Thread-Safety
    bool is_valid;         // Flag für sichere Iteration
} Node;

// Struktur für den AVL-Baum
typedef struct {
    Node *root;
    pthread_mutex_t tree_lock;  // Globaler Mutex für Baumoperationen
    size_t size;               // Anzahl der Knoten
} AVLTree;

// Hilfsfunktion: Maximum von zwei Zahlen
static inline int max(int a, int b) {
    return (a > b) ? a : b;
}

// Hilfsfunktion: Höhe eines Knotens
static int height(Node *node) {
    return node ? node->height : 0;
}

// Hilfsfunktion: Balancefaktor eines Knotens
static int balance_factor(Node *node) {
    return node ? height(node->left) - height(node->right) : 0;
}

// Sicheres Erstellen eines neuen Knotens
static Node* create_node(int key) {
    Node *node = (Node*)calloc(1, sizeof(Node));
    if (!node) {
        return NULL;
    }
    
    if (pthread_mutex_init(&node->lock, NULL) != 0) {
        free(node);
        return NULL;
    }
    
    node->key = key;
    node->height = 1;
    node->is_valid = true;
    return node;
}

// Rechtsdrehung
static Node* rotate_right(Node *y) {
    if (!y || !y->left) return y;
    
    Node *x = y->left;
    Node *T2 = x->right;
    
    x->right = y;
    y->left = T2;
    
    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;
    
    return x;
}

// Linksdrehung
static Node* rotate_left(Node *x) {
    if (!x || !x->right) return x;
    
    Node *y = x->right;
    Node *T2 = y->left;
    
    y->left = x;
    x->right = T2;
    
    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;
    
    return y;
}

// Initialisierung des AVL-Baums
AVLTree* avl_create(void) {
    AVLTree *tree = (AVLTree*)calloc(1, sizeof(AVLTree));
    if (!tree) {
        return NULL;
    }
    
    if (pthread_mutex_init(&tree->tree_lock, NULL) != 0) {
        free(tree);
        return NULL;
    }
    
    return tree;
}

// Rekursive Einfügefunktion mit Rebalancing
static Node* insert_recursive(Node *node, int key, bool *success) {
    if (!node) {
        Node *new_node = create_node(key);
        if (!new_node) {
            *success = false;
            return NULL;
        }
        *success = true;
        return new_node;
    }
    
    // Rekursives Einfügen
    if (key < node->key) {
        pthread_mutex_lock(&node->lock);
        node->left = insert_recursive(node->left, key, success);
        pthread_mutex_unlock(&node->lock);
    } else if (key > node->key) {
        pthread_mutex_lock(&node->lock);
        node->right = insert_recursive(node->right, key, success);
        pthread_mutex_unlock(&node->lock);
    } else {
        // Duplikate nicht erlaubt
        *success = false;
        return node;
    }
    
    // Höhe aktualisieren
    node->height = max(height(node->left), height(node->right)) + 1;
    
    // Balancefaktor berechnen und rebalancieren
    int balance = balance_factor(node);
    
    // Links-Links Fall
    if (balance > 1 && key < node->left->key) {
        return rotate_right(node);
    }
    
    // Rechts-Rechts Fall
    if (balance < -1 && key > node->right->key) {
        return rotate_left(node);
    }
    
    // Links-Rechts Fall
    if (balance > 1 && key > node->left->key) {
        pthread_mutex_lock(&node->lock);
        node->left = rotate_left(node->left);
        pthread_mutex_unlock(&node->lock);
        return rotate_right(node);
    }
    
    // Rechts-Links Fall
    if (balance < -1 && key < node->right->key) {
        pthread_mutex_lock(&node->lock);
        node->right = rotate_right(node->right);
        pthread_mutex_unlock(&node->lock);
        return rotate_left(node);
    }
    
    return node;
}

// Thread-sichere Einfügefunktion
bool avl_insert(AVLTree *tree, int key) {
    if (!tree) return false;
    
    pthread_mutex_lock(&tree->tree_lock);
    
    bool success = false;
    tree->root = insert_recursive(tree->root, key, &success);
    
    if (success) {
        tree->size++;
    }
    
    pthread_mutex_unlock(&tree->tree_lock);
    return success;
}

// Hilfsfunktion: Finde den Minimum-Knoten
static Node* find_min(Node *node) {
    if (!node) return NULL;
    while (node->left) {
        node = node->left;
    }
    return node;
}

// Rekursive Löschfunktion mit Rebalancing
static Node* delete_recursive(Node *node, int key, bool *success) {
    if (!node) {
        *success = false;
        return NULL;
    }
    
    // Rekursive Suche nach dem zu löschenden Knoten
    if (key < node->key) {
        pthread_mutex_lock(&node->lock);
        node->left = delete_recursive(node->left, key, success);
        pthread_mutex_unlock(&node->lock);
    } else if (key > node->key) {
        pthread_mutex_lock(&node->lock);
        node->right = delete_recursive(node->right, key, success);
        pthread_mutex_unlock(&node->lock);
    } else {
        // Knoten gefunden - Löschen
        *success = true;
        
        // Fall 1: Blattknoten oder nur ein Kind
        if (!node->left || !node->right) {
            Node *temp = node->left ? node->left : node->right;
            
            if (!temp) {
                // Kein Kind
                temp = node;
                node = NULL;
            } else {
                // Ein Kind
                *node = *temp;  // Kopiere Inhalt des Kindes
            }
            
            // Markiere als ungültig für sichere Iteration
            temp->is_valid = false;
            pthread_mutex_destroy(&temp->lock);
            free(temp);
        } else {
            // Fall 2: Zwei Kinder
            Node *temp = find_min(node->right);
            node->key = temp->key;
            
            pthread_mutex_lock(&node->lock);
            node->right = delete_recursive(node->right, temp->key, success);
            pthread_mutex_unlock(&node->lock);
        }
    }
    
    if (!node) return NULL;
    
    // Aktualisiere Höhe und rebalanciere
    node->height = max(height(node->left), height(node->right)) + 1;
    int balance = balance_factor(node);
    
    // Rebalancing-Fälle
    if (balance > 1 && balance_factor(node->left) >= 0) {
        return rotate_right(node);
    }
    
    if (balance > 1 && balance_factor(node->left) < 0) {
        pthread_mutex_lock(&node->lock);
        node->left = rotate_left(node->left);
        pthread_mutex_unlock(&node->lock);
        return rotate_right(node);
    }
    
    if (balance < -1 && balance_factor(node->right) <= 0) {
        return rotate_left(node);
    }
    
    if (balance < -1 && balance_factor(node->right) > 0) {
        pthread_mutex_lock(&node->lock);
        node->right = rotate_right(node->right);
        pthread_mutex_unlock(&node->lock);
        return rotate_left(node);
    }
    
    return node;
}

// Thread-sichere Löschfunktion
bool avl_delete(AVLTree *tree, int key) {
    if (!tree) return false;
    
    pthread_mutex_lock(&tree->tree_lock);
    
    bool success = false;
    tree->root = delete_recursive(tree->root, key, &success);
    
    if (success) {
        tree->size--;
    }
    
    pthread_mutex_unlock(&tree->tree_lock);
    return success;
}

// Thread-sichere Suchfunktion
bool avl_search(const AVLTree *tree, int key) {
    if (!tree) return false;
    
    pthread_mutex_lock(&tree->tree_lock);
    Node *current = tree->root;
    
    while (current && current->is_valid) {
        if (key == current->key) {
            pthread_mutex_unlock(&tree->tree_lock);
            return true;
        }
        current = (key < current->key) ? current->left : current->right;
    }
    
    pthread_mutex_unlock(&tree->tree_lock);
    return false;
}

// Rekursive Cleanup-Funktion
static void cleanup_recursive(Node *node) {
    if (!node) return;
    
    cleanup_recursive(node->left);
    cleanup_recursive(node->right);
    
    pthread_mutex_destroy(&node->lock);
    free(node);
}

// Aufräumfunktion für den gesamten Baum
void avl_destroy(AVLTree *tree) {
    if (!tree) return;
    
    pthread_mutex_lock(&tree->tree_lock);
    cleanup_recursive(tree->root);
    pthread_mutex_unlock(&tree->tree_lock);
    
    pthread_mutex_destroy(&tree->tree_lock);
    free(tree);
}

// Validierungsfunktion für Grenzen
bool avl_validate_bounds(const AVLTree *tree, int min_key, int max_key) {
    if (!tree || min_key > max_key) return false;
    
    pthread_mutex_lock(&tree->tree_lock);
    Node *current = tree->root;
    bool valid = true;
    
    // Prüfe rekursiv, ob alle Knoten innerhalb der Grenzen liegen
    while (current && current->is_valid) {
        if (current->key < min_key || current->key > max_key) {
            valid = false;
            break;
        }
        current = (current->key < max_key) ? current->right : current->left;
    }
    
    pthread_mutex_unlock(&tree->tree_lock);
    return valid;
}

// Iterator-Struktur für sichere Traversierung
typedef struct {
    const AVLTree *tree;
    Node *current;
    pthread_mutex_t iter_lock;
} AVLIterator;

// Iterator erstellen
AVLIterator* avl_iterator_create(const AVLTree *tree) {
    if (!tree) return NULL;
    
    AVLIterator *iter = (AVLIterator*)malloc(sizeof(AVLIterator));
    if (!iter) return NULL;
    
    if (pthread_mutex_init(&iter->iter_lock, NULL) != 0) {
        free(iter);
        return NULL;
    }
    
    iter->tree = tree;
    iter->current = tree->root;
    return iter;
}

// Zum nächsten Element bewegen (In-Order-Traversierung)
bool avl_iterator_next(AVLIterator *iter, int *key) {
    if (!iter || !key) return false;
    
    pthread_mutex_lock(&iter->iter_lock);
    pthread_mutex_lock(&iter->tree->tree_lock);
    
    if (!iter->current || !iter->current->is_valid) {
        pthread_mutex_unlock(&iter->tree->tree_lock);
        pthread_mutex_unlock(&iter->iter_lock);
        return false;
    }
    
    *key = iter->current->key;
    
    // Finde den nächsten Knoten (In-Order)
    if (iter->current->right) {
        iter->current = iter->current->right;
        while (iter->current->left) {
            iter->current = iter->current->left;
        }
    } else {
        Node *parent = NULL;
        Node *temp = iter->tree->root;
        while (temp != iter->current) {
            if (iter->current->key < temp->key) {
                parent = temp;
                temp = temp->left;
            } else {
                temp = temp->right;
            }
        }
        iter->current = parent;
    }
    
    pthread_mutex_unlock(&iter->tree->tree_lock);
    pthread_mutex_unlock(&iter->iter_lock);
    return true;
}

// Iterator aufräumen
void avl_iterator_destroy(AVLIterator *iter) {
    if (!iter) return;
    
    pthread_mutex_destroy(&iter->iter_lock);
    free(iter);
}