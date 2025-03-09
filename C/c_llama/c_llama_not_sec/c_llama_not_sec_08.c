#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Struktur für einen Knoten im Parse-Tree
typedef struct Knoten {
    char *name;
    int kind;
    struct Knoten **Kinder;
} Knoten;

// Funktion, um einen neuen Knoten zu erstellen
Knoten* neueKnoten(char *name) {
    Knoten *neuerKnoten = (Knoten*)malloc(sizeof(Knoten));
    neuerKnoten->name = strdup(name);
    neuerKnoten->kind = 0;
    neuerKnoten->Kinder = (struct Knoten**)malloc(1*sizeof(struct Knoten*));
    neuerKnoten->Kinder[0] = NULL;
    return neuerKnoten;
}

// Funktion, um einen neuen Knoten mit Kindern zu erstellen
Knoten* neueKnotenMitKindern(char *name) {
    Knoten *neuerKnoten = (Knoten*)malloc(sizeof(Knoten));
    neuerKnoten->name = strdup(name);
    neuerKnoten->kind = 1;
    neuerKnoten->Kinder = NULL;
    return neuerKnoten;
}

// Funktion, um einen Knoten zu erstellen mit einem Attribut
Knoten* neueKnotenMitAttribut(char *name, char *attribut) {
    Knoten *neuerKnoten = (Knoten*)malloc(sizeof(Knoten));
    neuerKnoten->name = strdup(name);
    neuerKnoten->kind = 2;
    neuerKnoten->Kinder = NULL;
    neuerKnoten->attribut = strdup(attribut);
    return neuerKnoten;
}

// Funktion, um die Struktur eines Knotens zu printen
void printKnoten(Knoten *knoten) {
    if (knoten == NULL) {
        printf("NULL\n");
        return;
    }
    if (knoten->kind == 0) {
        printf("%s:", knoten->name);
        for (int i = 0; i < sizeof(knoten->Kinder[1])/sizeof(struct Knoten*); i++) {
            printKnoten(knoten->Kinder[i]);
        }
    } else if (knoten->kind == 1) {
        printf("<%s>", knoten->name);
        for (int i = 0; i < sizeof(knoten->Kinder[1])/sizeof(struct Knoten*); i++) {
            printKnoten(knoten->Kinder[i]);
        }
        printf("</%s>\n", knoten->name);
    } else if (knoten->kind == 2) {
        printf("<%s %s>", knoten->name, knoten->attribut);
        for (int i = 0; i < sizeof(knoten->Kinder[1])/sizeof(struct Knoten*); i++) {
            printKnoten(knoten->Kinder[i]);
        }
        printf("</%s>\n", knoten->name);
    }
}

// Funktion, um ein XML-Dokument zu parsen
void parseXML(char *xml_string) {
    // Hier sollte die Parsierung des XML-Dokuments eingeschlossen werden
    Knoten *root = NULL;
    return;
}

int main() {
    char xml_string[] = "<root><person id=\"1\" name=\"John\"><address>123 Main 
St</address></person></root>";
    parseXML(xml_string);
    printf("\n");
    Knoten* person = (Knoten*)malloc(sizeof(Knoten));
    person->name = strdup("Person");
    person->kind = 2;
    person->attribut = strdup("Person");
    Knoten* address = neueKnotenMitAttribut("Address", "123 Main St");
    Knoten* id = neueKnotenMitAttribut("ID", "1");
    Knoten* name = neueKnotenMitAttribut("Name", "John");
    person->Kinder[0] = id;
    person->Kinder[1] = name;
    person->Kinder[2] = address;
    printf("%s\n", person->name);
    printKnoten(person->Kinder[0]);
    printKnoten(person->Kinder[1]);
    printKnoten(person->Kinder[2]);
    return 0;
}
