#include <stdio.h>

#define ADD(a, b) ((a) + (b))
#define SUB(a, b) ((a) - (b))
#define MUL(a, b) ((a) * (b))
#define DIV(a, b) ((a) / (b))
#define MOD(a, b) ((a) % (b))
#define AND(a, b) ((a) && (b))
#define OR(a, b) ((a) || (b))
#define NOT(a) (!(a))

#define EXPRESSION ADD(2, MUL(3, SUB(4, DIV(6, 2))))

int main() {
    int result = EXPRESSION;
    printf("Result: %d\n", result);
    return 0;
}
