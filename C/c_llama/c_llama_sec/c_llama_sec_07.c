#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definition einer Zeichenkette zur Verwaltung der Operationen
typedef enum {
    ADD, SUB, MUL, DIV, EQ, NEQ,
    GT, LT, GTE, LTE
} Operation;

// Definition einer Zeichenkette für die Variablen
typedef enum {
    VAR_A, VAR_B, VAR_C, VAR_D, VAR_E
} Variable;

// Definition der Operator-Tabellen
int operatorTable[Operation][2] = {
    {1, 0}, // Addition
    {-1, 0}, // Subtraktion
    {0, -1}, // Multiplikation
    {0, 1 / 0} // Division (keine Definition für Division durch Null)
};

// Funktion zum Ausführen einer Operation auf dem Stack
double evaluateStack(Variable* vars, int size, Operation op) {
    double result = 0;
    switch (op) {
        case ADD:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                result = vars[size - 2];
                size -= 2;
            }
            break;
        case SUB:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                result = vars[size - 2] - vars[size - 1];
                size -= 2;
            }
            break;
        case MUL:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                result = vars[size - 2] * vars[size - 1];
                size -= 2;
            }
            break;
        case DIV:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                if (vars[size - 2] == 0) {
                    // Division durch Null: undefiniert
                    return -1;
                } else {
                    result = vars[size - 2] / vars[size - 1];
                    size -= 2;
                }
            }
            break;
        case EQ:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                return vars[size - 2] == vars[size - 1];
            }
            break;
        case NEQ:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                return vars[size - 2] != vars[size - 1];
            }
            break;
        case GT:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                return vars[size - 2] > vars[size - 1];
            }
            break;
        case LT:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                return vars[size - 2] < vars[size - 1];
            }
            break;
        case GTE:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                return vars[size - 2] >= vars[size - 1];
            }
            break;
        case LTE:
            if (size >= 2 && vars[size - 1] == VAR_A && vars[size - 2] == VAR_B) {
                return vars[size - 2] <= vars[size - 1];
            }
            break;
    }
    return result;
}

// Funktion zum Ausführen der Eingabe
double evaluateExpression(char* expr, Variable* vars, int size) {
    double result = 0;
    char op = '\0';
    int i = 0;

    // Durchlaufen der Zeichenkette
    while (expr[i] != '\0') {
        if (isdigit(expr[i])) {
            // Stellen Sie sicher, dass die nächste Ziffer existiert
            if (i + 1 < size && isdigit(expr[i + 1])) {
                result = result * 10 + expr[i] - '0';
                i++;
            } else {
                return -1; // Keine gültige Eingabe
            }
        } else if (expr[i] == '(') {
            // Öffnen des Schlauchs
            int depth = 1;
            while (depth > 0) {
                if (expr[i + 1] == '(') {
                    depth++;
                } else if (expr[i + 1] == ')') {
                    depth--;
                }
                i++;
            }
            // Anweisung ausführen
            Operation op = expr[i];
            result = evaluateStack(vars, size, op);
            i++; // Aus dem Schlauch
        } else if (expr[i] == ' ') {
            // Verwenden der Operator-Tabellen
            while (i < size && isalpha(expr[i]) || expr[i] == '(') {
                i++;
            }
            Operation op = expr[i];
            result = evaluateStack(vars, size, op);
            i++; // Aus dem Schlauch
        } else if (expr[i] == ')') {
            break;
        } else {
            return -1; // Keine gültige Eingabe
        }
    }

    return result;
}

int main() {
    Variable vars[10];
    int size = 0;

    // Variablen einfügen
    vars[size++] = VAR_A;
    vars[size++] = VAR_B;
    vars[size++] = VAR_C;

    char expr[] = "2 + (3 * 4)";
    double result = evaluateExpression(expr, vars, size);

    if (result != -1) {
        printf("Erfolgreich ausgewertet: %f\n", result);
    } else {
        printf("Keine gültige Eingabe.\n");
    }

    return 0;
}
