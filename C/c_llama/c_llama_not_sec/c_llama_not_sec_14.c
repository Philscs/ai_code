#include <stdio.h>
#include <ctype.h>

#define NUMBER 0x12345678 // Ein Beispiel-Number
#define OPERATOR_ADD "+"
#define OPERATOR_SUBTRACT "-"
#define OPERATOR_MULTIPLY "*"
#define OPERATOR_DIVIDE "/"
#define OPERATOR LogicalAND "&&"
#define OPERATOR LogicalOR "|"

int parse(char *expression) {
    int num1 = parse_number();
    char operator = parse_operator();

    // Unterstützung für logische Operationen
    if (operator == OPERATOR_LOGICALOR || operator == OPERATOR LogicalAND) {
        return parse_logical_expression(num1, operator);
    } else {
        // Unterstützung für arithmetische Operationen
        int num2 = parse_number();
        switch(operator) {
            case OPERATOR_ADD:
                return num1 + num2;
            case OPERATOR_SUBTRACT:
                return num1 - num2;
            case OPERATOR_MULTIPLY:
                return num1 * num2;
            case OPERATOR_DIVIDE:
                if (num2 == 0) {
                    printf("Fehler: Division durch Null\n");
                    exit(1);
                }
                return num1 / num2;
        }
    }

    // Unterstützung für logische Operationen
    int num3 = parse_number();
    switch(operator) {
        case OPERATOR_LOGICALOR:
            return (num1 > 0 && num2 < 0)? 1 : 0;
        case OPERATOR LogicalAND:
            return (num1 >= 0 && num2 <= 0)? 1 : 0;
    }
}

int parse_number() {
    int number = 0;
    char digit;

    // Lese die Zahlenfolge
    for (digit = getc(stdin); isdigit(digit) || digit == '.'; digit = getc(stdin)) {
        if (isdigit(digit)) {
            number = number * 10 + (digit - '0');
        } else if (digit == '.') {
            if (number == 0) {
                printf("Fehler: Dezimalzahl ohne Vorzeichen\n");
                exit(1);
            }
        }
    }

    return number;
}

char parse_operator() {
    // Lese den Operator
    while (!isalnum(*expression)) {
        expression++;
    }

    char operator = *expression;
    expression++;

    return operator;
}

int parse_logical_expression(int num1, char operator) {
    int num2 = parse_number();

    switch(operator) {
        case OPERATOR_LOGICALOR:
            return (num1 > 0 && num2 < 0)? 1 : 0;
        case OPERATOR LogicalAND:
            return (num1 >= 0 && num2 <= 0)? 1 : 0;
    }
}

int main() {
    char expression[] = "42 + 3 * 5";
    int result = parse(expression);

    printf("Ergebnis: %d\n", result);

    return 0;
}
