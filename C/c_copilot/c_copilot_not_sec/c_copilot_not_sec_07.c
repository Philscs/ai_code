#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Define the NFA structure
typedef struct NFA {
    int startState;
    int acceptState;
} NFA;

// Function to create a basic NFA for a single character
NFA createBasicNFA(char c) {
    NFA nfa;
    nfa.startState = 0;
    nfa.acceptState = 1;
    
    // Print the NFA transitions
    printf("NFA transitions for '%c':\n", c);
    printf("(0, '%c') -> 1\n", c);
    
    return nfa;
}

// Function to concatenate two NFAs
NFA concatenate(NFA nfa1, NFA nfa2) {
    NFA nfa;
    nfa.startState = nfa1.startState;
    nfa.acceptState = nfa2.acceptState;
    
    // Print the NFA transitions
    printf("NFA transitions for concatenation:\n");
    printf("(start1, epsilon) -> start2\n");
    printf("(accept1, epsilon) -> start2\n");
    printf("(accept1, epsilon) -> accept2\n");
    
    return nfa;
}

// Function to union two NFAs
NFA unionNFA(NFA nfa1, NFA nfa2) {
    NFA nfa;
    nfa.startState = 0;
    nfa.acceptState = 3;
    
    // Print the NFA transitions
    printf("NFA transitions for union:\n");
    printf("(start, epsilon) -> start1\n");
    printf("(start, epsilon) -> start2\n");
    printf("(accept1, epsilon) -> accept\n");
    printf("(accept2, epsilon) -> accept\n");
    
    return nfa;
}

// Function to apply the Kleene star operation on an NFA
NFA kleeneStar(NFA nfa) {
    NFA newNFA;
    newNFA.startState = 0;
    newNFA.acceptState = 2;
    
    // Print the NFA transitions
    printf("NFA transitions for Kleene star:\n");
    printf("(start, epsilon) -> start1\n");
    printf("(start, epsilon) -> accept\n");
    printf("(accept1, epsilon) -> start1\n");
    printf("(accept1, epsilon) -> accept\n");
    
    return newNFA;
}

int main() {
    // Create NFAs for individual characters
    NFA nfa1 = createBasicNFA('a');
    NFA nfa2 = createBasicNFA('b');
    
    // Concatenate the NFAs
    NFA concatenatedNFA = concatenate(nfa1, nfa2);
    
    // Union the NFAs
    NFA unionedNFA = unionNFA(nfa1, nfa2);
    
    // Apply Kleene star operation
    NFA kleeneStarNFA = kleeneStar(nfa1);
    
    return 0;
}
