#include <stdio.h>
#include <stdlib.h>

// Typen für unsere Subset-Implementierung
typedef struct {
    int value;
} Value;

typedef struct {
    char *name;
    int (*function)(Value, Value);
} Function;

// Variable Deklaration
void var_decl(int value) {
    // Variable erstellen und speichern
}

// Funktion Deklaration
Function func_decl(char *name) {
    // Funktion erstellen und speichern
}

// Ausdrücke (Basisoperationen +)
Value eval(Value a, Value b) {
    // Basisoperation '+' implementieren
    return {a.value + b.value};
}

// Funktionrufe
void call(Function func, Value args) {
    // Funktion aufrufen und Argument übergeben
}

// JIT-Compiler
int main() {
    Function add = func_decl("add");
    Value *x = var_decl(5);
    Value *y = var_decl(3);

    call(add, *x, *y); // Ergebnis: 8

    return 0;
}
