#include <stdio.h>
#include <stdlib.h>

// Strukttype für einen Knoten im Rot-Black-Tree
typedef struct Knoten {
    int matrikelnummer;
    char name[100];
    double notendurchschnitt;
    struct Knoten *linkes;
    struct Knoten *rechts;
    int rot;
} Knoten;

// Funktion, um einen neuen Knoten zu erstellen
Knoten* neu_Knoten(int matrikelnummer, char* name, double notendurchschnitt) {
    Knoten* knoten = (Knoten*)malloc(sizeof(Knoten));
    knoten->matrikelnummer = matrikelnummer;
    strcpy(knoten->name, name);
    knoten->notendurchschnitt = notendurchschnitt;
    knoten->linkes = NULL;
    knoten->rechts = NULL;
    knoten->rot = 0; // Knoten ist rot

    return knoten;
}

// Funktion, um einen Knoten im Rot-Black-Tree zu erstellen
Knoten* rot_black_tree_neu() {
    return neu_Knoten(0, "", 0.0);
}

// Funktion, um einen Knoten zu kopieren
Knoten* rot_black_tree_kopie(Knoten* knoten) {
    Knoten* neu = neu_Knoten(knoten->matrikelnummer, knoten->name, knoten->notendurchschnitt);

    if (knoten->linkes != NULL)
        neu->linkes = rot_black_tree_kopie(knoten->linkes);
    if (knoten->rechts != NULL)
        neu->rechts = rot_black_tree_kopie(knoten->rechts);

    return neu;
}

// Funktion, um einen Knoten zu ersetzen
void rot_black_tree_ersetze(Knoten** knoten, Knoten* neu) {
    if (*knoten == NULL)
        *knoten = neu;
    else {
        if ((*knoten)->linkes == NULL || (*knoten)->rechts == NULL) {
            if ((*knoten)->linkes == NULL)
                (*knoten)->linkes = neu;
            else
                (*knoten)->rechts = neu;
        } else {
            rot_black_tree_umordnen(*knoten, neu);
            if ((*knoten)->rot == 1) // Knoten links ist rot und neu rechts
                (*knoten)->linkes->rot = 0;
            else // Knoten rechts ist rot und neu links
                (*knoten)->rechts->rot = 0;
        }
    }
}

// Funktion, um den Knoten zu ersetzen, ohne die Links zu bearbeiten
void rot_black_tree_umordnen(Knoten* knoten, Knoten* neu) {
    if (neu == NULL)
        return;

    while ((knoten != NULL) && (neu != NULL)) {
        if (knoten->matrikelnummer < neu->matrikelnummer) {
            Knoten* temp = neu;
            neu = knoten;
            knoten = temp;
        }
        Knoten* wuchsRechts = knoten->rechts;
        Knoten* wuchsLinks = knoten->linkes;

        if (wuchsLinks == NULL || wuchsRechts == NULL) {
            rot_black_tree_ersetze(&knoten, neu);
            break;
        }

        while ((neu != NULL) && (wuchsRechts != NULL)) {
            Knoten* temp = neu;
            neu = wuchsRechts->rechts;
            wuchsRechts->rechts = temp;
        }
        if (neu == NULL)
            rot_black_tree_ersetze(&knoten, wuchsRechts);
        else
            rot_black_tree_ersetze(&knoten, neu);

        Knoten* temp = wuchsLinks;
        wuchsLinks = knoten->linkes;
        knoten->linkes = temp;

        if (wuchsRechts != NULL)
            wuchsRechts->rot = 0;
    }
}

// Funktion, um den Rot-Black-Tree zu manipulieren
void rot_black_tree_insert(Knoten** knoten, int matrikelnummer, char* name, double 
notendurchschnitt) {
    *knoten = rot_black_tree_neu();
    Knoten* wuchsRechts = (*knoten);
    Knoten* wuchsLinks = NULL;

    while (wuchsRechts != NULL) {
        if (matrikelnummer < wuchsRechts->matrikelnummer)
            wuchsRechts = wuchsRechts->linkes;
        else
            break;
    }

    if (wuchsRechts == NULL) {
        *knoten = rot_black_tree_kopie(wuchsRechts);
        return;
    }

    while (wuchsLinks != NULL && matrikelnummer < wuchsLinks->matrikelnummer)
        wuchsLinks = wuchsLinks->linkes;

    if (wuchsLinks == NULL) {
        Knoten* temp = rot_black_tree_kopie(wuchsRechts);
        rot_black_tree_ersetze(&(*knoten), temp);
    } else {
        rot_black_tree_umordnen(wuchsRechts, wuchsRechts->rechts);

        Knoten* neu = rot_black_tree_neu();
        Knoten* wuchsTemp = neu;
        Neu wuchsLinksTemp;

        if (matrikelnummer < wuchsRechts->matrikelnummer) {
            wuchsNeu = wuchsNeu->linkes;
            neu = wuchsRechs;
        } else {
            wuchsLinkTemp = rot_black_tree_kopie(wchsLink);
            wchsTemp->links = wchslinkTemp;

            Knoten* temp = wchsLink->rechts;
            wchsLink->rechts = neu;
            neu = temp;
        }

        if (wchsNeu == NULL)
            wchsNeu = rot_black_tree_kopie(wchsRecht);
        else
            wchsRecht->rot = 1;

        Knoten* temp = neu;
        wchsRecht = wchsRecht->rechts;
        neu = temp;

        while (temp != NULL) {
            wchsTemp->links = neu;
            Neu wchsNeu;
            if (wchsLinkTemp == NULL || wchsRecht->matrikelnummer < wchsLinkTemp->matrikelnummer)
                wchsNeu = rot_black_tree_kopie(wchsRight);
            else
                wchsNeu = wchsLink;

            Knoten* neuTemp = wchsTemp->rechts;
            wchsTemp->rechts = wchsRecht;
            wchsRight = temp;
            temp = neuTemp;
        }
    }

    rot_black_tree_ersetze(&(*knoten), wuchsLinks);
}

// Funktion, um den Rot-Black-Tree zu drucken
void rot_black_tree_druck(Knoten* knoten) {
    if (knoten == NULL)
        return;

    printf("(" %d ", %s ", %.2lf")", knoten->matrikelnummer, knoten->name, 
knoten->notendurchschnitt);
    rot_black_tree_druck(knoten->linkes);
    rot_black_tree_druck(knoten->rechts);

}

int main() {
    Knoten* knoten = NULL;

    int matrikelnummern[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char* names[10] = {"Max", "Lena", "Tom", "Anna", "Markus", "Sophia", "Dmitri", "Luisa", 
"Alexander", "Julia"};
    double notendurchschnittwerte[10] = {3.5, 4.2, 3.8, 4.1, 3.9, 4.0, 3.7, 4.3, 3.6, 4.4};

    for (int i = 0; i < 10; i++) {
        rot_black_tree_insert(&knoten, matrikelnummern[i], names[i], notendurchschnittwerte[i]);
    }

    printf("Rot-Black-Tree:\n");
    rot_black_tree_druck(knoten);

    return 0;
}
