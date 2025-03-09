#include <stdio.h>

// Metaprogramming-Hilfsmakros
#define EVAL(...) EVAL1024(__VA_ARGS__)
#define EVAL1024(...) EVAL512(EVAL512(__VA_ARGS__))
#define EVAL512(...) EVAL256(EVAL256(__VA_ARGS__))
#define EVAL256(...) EVAL128(EVAL128(__VA_ARGS__))
#define EVAL128(...) EVAL64(EVAL64(__VA_ARGS__))
#define EVAL64(...) EVAL32(EVAL32(__VA_ARGS__))
#define EVAL32(...) EVAL16(EVAL16(__VA_ARGS__))
#define EVAL16(...) EVAL8(EVAL8(__VA_ARGS__))
#define EVAL8(...) EVAL4(EVAL4(__VA_ARGS__))
#define EVAL4(...) EVAL2(EVAL2(__VA_ARGS__))
#define EVAL2(...) EVAL1(EVAL1(__VA_ARGS__))
#define EVAL1(...) __VA_ARGS__

// Operator Definitionen
#define ADD(x, y) x + y
#define SUB(x, y) x - y
#define MUL(x, y) x * y
#define DIV(x, y) x / y
#define MOD(x, y) x % y

#define AND(x, y) x && y
#define OR(x, y) x || y
#define NOT(x) !(x)
#define EQ(x, y) x == y
#define NEQ(x, y) x != y
#define LT(x, y) x < y
#define GT(x, y) x > y
#define LTE(x, y) x <= y
#define GTE(x, y) x >= y

// Hilfsmakros für bedingte Auswertung
#define IF(cond, then, else) (cond ? then : else)
#define WHEN(cond, then) IF(cond, then, )
#define UNLESS(cond, then) IF(NOT(cond), then, )

// Rekursive Expression-Auswertung
#define EXPR(...) EVAL(__VA_ARGS__)

// Beispielnutzung
#define TEST_EXPR1 EXPR(ADD(MUL(2, 3), DIV(10, 2)))  // 2*3 + 10/2 = 11
#define TEST_EXPR2 EXPR(AND(GT(5, 3), NOT(EQ(2, 2)))) // (5>3) && !(2==2) = 0

int main() {
    // Compile-Time Konstanten
    enum {
        Result1 = TEST_EXPR1,
        Result2 = TEST_EXPR2
    };
    
    printf("Test Expression 1 Result: %d\n", Result1);
    printf("Test Expression 2 Result: %d\n", Result2);
    
    // Beispiel für bedingte Kompilierung
    #if EXPR(GT(TEST_EXPR1, 10))
        printf("Expression 1 is greater than 10\n");
    #endif
    
    return 0;
}