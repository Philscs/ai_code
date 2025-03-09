#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Definiere Strukturen für den Rot-Grünen-Baum
typedef struct Knoten {
    int key;
    char color; // rot/grün
    struct Knoten* linkL;
    struct Knoten* linkR;
} Knoten;

// Definiert die Konstanten für den Rot-Grünen-Baum
#define ROT 1
#define GRÜN 0

typedef struct Baum {
    Knoten* root;
    pthread_mutex_t mutex;
} Baum;

// Funktion zum Erstellen eines neuen Knotens
Knoten* neuKnoten(int key) {
    Knoten* neuerKnoten = malloc(sizeof(Knoten));
    if (!neuerKnoten) {
        printf("Fehler beim Erstellen eines neuen Knotens\n");
        exit(1);
    }
    neuerKnoten->key = key;
    neuerKnoten->color = ROT;
    return neuerKnoten;
}

// Funktion zum Finden des Rot-Grünen-Baums
Baum* rotGrüneBaum(int rootKey) {
    Baum* neuBaum = malloc(sizeof(Baum));
    if (!neuBaum) {
        printf("Fehler beim Erstellen eines neuen Baumes\n");
        exit(1);
    }
    neuBaum->root = rootKey;
    pthread_mutex_init(&neuBaum->mutex, NULL);

    return neuBaum;
}

// Funktion zum Anordnen des Rot-Grünen-Baums
void anordnen(Knoten* knoten) {
    if (knoten->linkL == NULL && knoten->linkR == NULL) {
        // Kein Kind
        return;
    }

    if (knoten->linkL != NULL && knoten->linkR != NULL) {
        // Kinder
        anordnen(knoten->linkL);
        anordnen(knoten->linkR);
        return;
    }

    if (knoten->linkL == NULL) {
        Knoten* linkR = knoten->linkR;
        knoten->key = linkR->key;
        knoten->color = linkR->color;
        // Knoten neu anordnen
        return;
    } else {
        Knoten* linkR = knoten->linkR;
        if (knoten->color == ROT) {
            // Knoten ist rot, umkehren
            Knoten* linkL = knoten->linkL;

            // Umkehrung nach oben gehen
            while (linkL != NULL && linkL->color == ROT) {
                if (linkL->linkR == NULL || linkL->linkR->color == GRÜN)
                    break;
                // Knoten links umkehren
                knoten = linkL;
                linkL = knoten->linkL;
            }

            // Knoten mit dem rotesten Elternteil finden
            if (knoten == NULL || knoten->linkR == NULL) {
                knoten->color = GRÜN;
                return;
            } else {
                while (knoten->linkR != NULL && knoten->linkR->color == ROT)
                    knoten = knoten->linkR;

                // Knoten mit dem rotesten Elternteil finden
                if (knoten->linkR == NULL || knoten->linkR->color == GRÜN) {
                    knoten->color = GRÜN;
                    knoten->linkR->color = ROT;
                    return;
                } else {
                    Knoten* p = knoten->linkR;
                    Knoten* q = p->linkL;

                    // Rot-Grünen-Baum-Austausch
                    knoten->key = p->key;
                    knoten->color = p->color;
                    p->key = q->key;
                    p->color = q->color;

                    // Knoten neu anordnen
                    if (q != NULL) {
                        p->linkL = q;
                        p->linkR = q->linkR;
                        if (q->linkR != NULL)
                            q->linkR->linkL = p;
                    }

                    // Knoten neu anordnen
                    return;
                }
            }
        } else {
            // Knoten ist grün, nach unten gehen
            Knoten* linkR = knoten->linkR;

            while (linkR != NULL && linkR->color == ROT) {
                if (linkR->linkL == NULL || linkR->linkL->color == GRÜN)
                    break;
                // Knoten rechts umkehren
                knoten = linkR;
                linkR = knoten->linkR;
            }

            // Knoten mit dem rotesten Nachbarn finden
            if (knoten == NULL || knoten->linkL == NULL) {
                knoten->color = ROT;
                return;
            } else {
                while (knoten->linkL != NULL && knoten->linkL->color == ROT)
                    knoten = knoten->linkL;

                // Knoten mit dem rotesten Nachbarn finden
                if (knoten->linkL == NULL || knoten->linkL->color == GRÜN) {
                    knoten->color = ROT;
                    knoten->linkL->color = GRÜN;
                    return;
                } else {
                    Knoten* p = knoten->linkL;
                    Knoten* q = p->linkR;

                    // Rot-Grünen-Baum-Austausch
                    knoten->key = p->key;
                    knoten->color = p->color;
                    p->key = q->key;
                    p->color = q->color;

                    if (q != NULL) {
                        Knoten* temp = p->linkL;
                        p->linkL = q;
                        if (temp != NULL)
                            temp->linkR = p;
                    }

                    // Knoten neu anordnen
                    return;
                }
            }
        }
    }
}

// Funktion zum Hinzufügen eines neuen Knotens im Rot-Grünen-Baum
void hinzufügen(Knoten* knoten, int key) {
    pthread_mutex_lock(&knoten->mutex);
    // Knoten neu anordnen
    Knoten* temp = anordnen(knoten);

    if (temp == NULL)
        printf("Fehler beim Hinzufügen eines neuen Knotens\n");

    if (key < knoten->key) {
        Knoten* newKnoten = neuKnoten(key);
        newKnoten->linkL = knoten;
        newKnoten->linkR = NULL;

        // Knoten neu anordnen
        Knoten* temp2 = anordnen(newKnoten);

        if (temp2 == NULL)
            printf("Fehler beim Hinzufügen eines neuen Knotens\n");

        // Knoten neu einfügen
        knoten->linkR = newKnoten;
    } else {
        Knoten* newKnoten = neuKnoten(key);
        newKnoten->linkL = NULL;
        newKnoten->linkR = knoten;

        // Knoten neu anordnen
        Knoten* temp2 = anordnen(newKnoten);

        if (temp2 == NULL)
            printf("Fehler beim Hinzufügen eines neuen Knotens\n");

        // Knoten neu einfügen
        knoten->linkL = newKnoten;
    }

    pthread_mutex_unlock(&knoten->mutex);
}

// Funktion zum Lösen eines Knotens im Rot-Grünen-Baum
void löschen(Knoten* knoten, int key) {
    pthread_mutex_lock(&knoten->mutex);

    if (knoten == NULL)
        printf("Fehler beim Löschen eines Knotens\n");

    // Knoten neu einordnen
    Knoten* temp = anordnen(knoten);

    if (temp == NULL)
        printf("Fehler beim Löschen eines Knotens\n");

    // Knoten neu einfügen
    Knoten* newKnoten = NULL;
    Knoten* knoten2 = NULL;

    if (knoten->key == key) {
        // Knoten mit dem gleichen Schlüssel finden
        newKnoten = knoten;

        // Links- und Rechtsnebenknoten finden
        if (newKnoten->linkL != NULL)
            knoten2 = anordnen(newKnoten->linkL);

        if (newKnoten->linkR != NULL)
            knoten2 = anordnen(newKnoten->linkR);

        // Knoten neu einfügen
        newKnoten->linkL = knoten2;

        // Knoten neu einordnen
        temp = anordnen(knoten);
    }

    if (temp == NULL)
        printf("Fehler beim Löschen eines Knotens\n");

    pthread_mutex_unlock(&kotinen->mutex);

    // Knoten löschen
    delete newKnoten;
}

// Funktion zum Ersetzen eines Knotens im Rot-Grünen-Baum
void ersetzen(Knoten* knoten, int key) {
    pthread_mutex_lock(&knoten->mutex);
    if (knoten == NULL)
        printf("Fehler beim Ersätzen eines Knotens\n");

    // Knoten neu einordnen
    Knoten* temp = anordnen(knoten);

    if (temp == NULL)
        printf("Fehler beim Ersätzen eines Knotens\n");
    else {
        if (key < knoten->key) {
            Knoten* newKnoten = neuKnoten(key);
            newKnoten->linkL = temp;
            Knoten* knoten2 = anordnen(newKnoten);

            if (knoten2 == NULL)
                printf("Fehler beim Ersätzen eines Knotens\n");

            Knoten* knoten3 = NULL;

            if (newKnoten->linkR != NULL) {
                newKnoten->linkR->linkL = temp;
                knoten3 = anordnen(newKnoten->linkR);
            }

            Knoten* temp2 = anordnen(temp);

            if (temp2 == NULL)
                printf("Fehler beim Ersätzen eines Knotens\n");

            // Knoten neu einfügen
            temp->linkL = newKnoten;
            knoten3 = anordnen(temp);

            // Knoten neu einordnen
            if (knoten3 != NULL)
                printf("Fehler beim Ersätzen eines Knotens\n");
        } else {
            Knoten* newKnoten = neuKnoten(key);
            newKnoten->linkR = temp;
            Knoten* knoten2 = anordnen(newKnoten);

            if (knoten2 == NULL)
                printf("Fehler beim Ersätzen eines Knotens\n");

            Knoten* knoten3 = NULL;

            if (newKnoten->linkL != NULL) {
                newKnoten->linkL->linkR = temp;
                knoten3 = anordnen(newKnoten->linkL);
            }

            Knoten* temp2 = anordnen(temp);

            if (temp2 == NULL)
                printf("Fehler beim Ersätzen eines Knotens\n");

            // Knoten neu einfügen
            temp->linkR = newKnoten;
            knoten3 = anordnen(temp);

            // Knoten neu einordnen
            if (knoten3 != NULL)
                printf("Fehler beim Ersätzen eines Knotens\n");
        }
    }

    pthread_mutex_unlock(&kotinen->mutex);
}

// Funktion zum Durchsuchen des Rot-Grünen-Baums nach einem bestimmten Schlüssel
int durchsuchen(Knoten* knoten, int key) {
    pthread_mutex_lock(&knoten->mutex);

    if (knoten == NULL)
        printf("Fehler beim Durchschauen des Rot-Grünen-Baums\n");

    // Knoten neu einordnen
    Knoten* temp = anordnen(knoten);

    if (temp == NULL)
        printf("Fehler beim Durchschauen des Rot-Grünen-Baums\n");

    // Knoten neu einfügen
    Knoten* newKnoten = NULL;
    Knoten* knoten2 = NULL;

    while (knoten != NULL && key > knoten->key) {
        if (newKnoten == NULL)
            newKnoten = knoten;

        if (knoten->linkR != NULL)
            knoten2 = anordnen(knoten->linkR);

        knoten = knoten2;
    }

    pthread_mutex_unlock(&kotinen->mutex);

    // Knoten neu einfügen
    return newKnoten == NULL ? -1 : newKnoten->key;
}

// Funktion zum Hinzufügen eines neuen Knotens mit der Klasse "Node"
void hinzufügen_node(Node* node, int key) {
    pthread_mutex_lock(&node->mutex);

    // Knoten neu anordnen
    Node* temp = anordenen(node);

    if (temp == NULL)
        printf("Fehler beim Hinzufügen eines neuen Knotens\n");

    pthread_mutex_unlock(&node->mutex);
}

// Funktion zum Ersetzen eines Knotens mit der Klasse "Node"
void ersetze_node(Node* node, int key) {
    pthread_mutex_lock(&node->mutex);

    if (key < node->key) {
        Node* newKnoten = neuKnoten(key);
        newKnoten->linkL = temp;
        Node* knoten2 = anordenen(newKnoten);

        if (knoten2 == NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");

        Node* knoten3 = NULL;

        if (newKnoten->linkR != NULL) {
            newKnoten->linkR->linkL = temp;
            knoten3 = anordenen(newKnoten->linkR);
        }

        Node* temp2 = anordenen(temp);

        if (temp2 == NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");

        // Knoten neu einfügen
        temp->linkL = newKnoten;
        knoten3 = anordenen(temp);

        // Knoten neu einfügen
        if (knoten3 != NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");
    } else {
        Node* newKnoten = neuKnoten(key);
        newKnoten->linkR = temp;
        Node* knoten2 = anordenen(newKnoten);

        if (knoten2 == NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");

        Node* knoten3 = NULL;

        if (newKnoten->linkL != NULL) {
            newKnoten->linkL->linkR = temp;
            knoten3 = anordenen(newKnoten->linkL);
        }

        Node* temp2 = anordenen(temp);

        if (temp2 == NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");

        // Knoten neu einfügen
        temp->linkR = newKnoten;
        knoten3 = anordenen(temp);

        // Knoten neu einfügen
        if (knoten3 != NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");
    }

    pthread_mutex_unlock(&node->mutex);
}

// Funktion zum Durchsuchen des Baums nach einem bestimmten Schlüssel
int durchsuche_node(Node* node, int key) {
    pthread_mutex_lock(&node->mutex);

    if (node == NULL)
        printf("Fehler beim Durchschauen des Baumes\n");

    // Knoten neu einordnen
    Node* temp = anordenen(node);

    if (temp == NULL)
        printf("Fehler beim Durchschauen des Baumes\n");
    else {
        while (key > node->key) {
            if (node->linkR != NULL)
                node = anordenen(node->linkR);
        }
    }

    pthread_mutex_unlock(&node->mutex);

    // Knoten neu einfügen
    return node == NULL ? -1 : node->key;
}

// Funktion zum Ersetzen eines Knotens mit der Klasse "Node"
void ersetze_node(Node* node, int key) {
    pthread_mutex_lock(&node->mutex);
    if (key < node->key) {
        Node* newKnoten = neuKnoten(key);
        newKnoten->linkL = temp;
        Node* knoten2 = anordenen(newKnoten);

        if (knoten2 == NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");

        Node* knoten3 = NULL;

        if (newKnoten->linkR != NULL) {
            newKnoten->linkR->linkL = temp;
            knoten3 = anordenen(newKnoten->linkR);
        }

        Node* temp2 = anordenen(temp);

        if (temp2 == NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");

        // Knoten neu einfügen
        temp->linkL = newKnoten;
        knoten3 = anordenen(temp);

        // Knoten neu einfügen
        if (knoten3 != NULL)
            printf("Fehler beim Ersätzen eines Knotens\n");
    } else {
        Node* newKnoten = neuKotnen(key);
        newKnoten->linkR = temp;
        Node* knoten2 = anordenen(newKnoten);

        if (knoten2 == NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");

        Node* knoten3 = NULL;

        if (newKotnen->linkL != NULL) {
            newKotnen->linkL->linkR = temp;
            knoten3 = anordenen(newKnoten->linkL);
        }

        Node* temp2 = anordenen(temp);

        if (temp2 == NULL)
            printf("Fehler beim Ersetzen eines Knotens\n");

        // Knoten neu einfügen
        temp->linkR = newKotnen;
        knoten3 = anordenen(temp);

        // Knoten neu einfügen
        if (knoten3 != NULL)
            printf("Fehler beim Ersätzen eines Knotens\n");
    }

    pthread_mutex_unlock(&node->mutex);
}
