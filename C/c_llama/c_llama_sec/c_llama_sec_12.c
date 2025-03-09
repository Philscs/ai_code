#include <stdio.h>
#include <stdlib.h>

// Funktion zur Eingabe eines Namens
void eingabe_name(char *name) {
    printf("Bitte geben Sie Ihren Namen ein: ");
    scanf("%s", name);
}

// Funktion zum Hinzufügen von Zahlen
int addiere_zahlen(int a, int b) {
    return a + b;
}

// Funktion zur Ausgabe des Ergebnisses
void ausgabe_resultat(int resultat) {
    printf("Das Ergebnis ist: %d\n", resultat);
}

// Funktion zur Überprüfung eines Namens
int überprüfe_name(char *name) {
    if (strlen(name) < 3) {
        return 0; // Name ist zu kurz
    } else if (strlen(name) > 10) {
        return 1; // Name ist zu lang
    } else {
        return 2; // Name ist korrekt
    }
}

int main() {
    char name[50];
    int a = 5;
    int b = 7;

    eingabe_name(name);

    int resultat = addiere_zahlen(a, b);
    ausgabe_resultat(resultat);

    if (überprüfe_name(name) == 2) {
        printf("Der Name ist korrekt.\n");
    } else {
        printf("Der Name ist nicht korrekt.\n");
    }

    return 0;
}
