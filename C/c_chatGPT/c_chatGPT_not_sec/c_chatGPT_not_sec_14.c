#include <stdio.h>
#include <stdbool.h>

// Function prototypes for logical operations
bool and(bool x, bool y) {
    return x && y;
}

bool or(bool x, bool y) {
    return x || y;
}

bool not(bool x) {
    return !x;
}

// Function prototypes for arithmetic operations
int add(int x, int y) {
    return x + y;
}

int sub(int x, int y) {
    return x - y;
}

int mul(int x, int y) {
    return x * y;
}

int divide(int x, int y) {
    return y != 0 ? x / y : 0; // Avoid division by zero
}

int mod(int x, int y) {
    return y != 0 ? x % y : 0;
}

// Function to evaluate an example expression
int eval_example1() {
    return add(5, mul(3, 2)); // 5 + (3 * 2)
}

bool eval_example2() {
    return and(true, or(false, not(false))); // true && (false || !false)
}

int eval_example3() {
    return divide(10, 2); // 10 / 2
}

int eval_example4() {
    return mod(10, 3); // 10 % 3
}

int main() {
    // Print results of evaluated expressions
    printf("Result of eval_example1: %d\n", eval_example1());
    printf("Result of eval_example2: %d\n", eval_example2());
    printf("Result of eval_example3: %d\n", eval_example3());
    printf("Result of eval_example4: %d\n", eval_example4());

    return 0;
}
