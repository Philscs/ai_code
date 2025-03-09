#include <stdint.h>
#include <stdbool.h>

// Typ für eine Funktion, die alsCoroutine ausgeführt werden kann
typedef void (*CoroutineFunction)(void*);

// Strukt für eine Coroutine
struct Coroutine {
    // Speicherblock für den Stack der Coroutine
    void* stack;
    
    // Kontext-Adresse für die Coroutine (z.B. Parameter)
    void* context;
    
    // Zustandsart der Coroutine (0: Laufend, 1: Warte, -1: Abgeschlossen)
    int state;

    // Adresse der nächsten Funktion, die ausgeführt werden soll
    CoroutineFunction next_function;
};

// Funktion zur Erstellung einer neuen Coroutine
Coroutine* coroutine_new(void* stack, void* context) {
    Coroutine* coroutine = malloc(sizeof(Coroutine));
    
    if (coroutine == NULL) {
        return NULL; // Ausfallfall: Nicht genug Speicher
    }
    
    coroutine->stack = stack;
    coroutine->context = context;
    coroutine->state = 0; // Anfangsstate ist Laufend
    
    return coroutine;
}

// Funktion zum Hinzufügen einer neuen Funktion zur Coroutine
void coroutine_add_function(Coroutine* coroutine, CoroutineFunction function) {
    coroutine->next_function = function;
}

int main() {
    // Erstelle einen neuen Speicherblock für den Stack
    void* stack = malloc(1024 * 1024); // 1 MB
    
    if (stack == NULL) {
        return 1; // Ausfallfall: Nicht genug Speicher
    }
    
    // Erstelle eine neue Coroutine mit einem gegebenen Kontext-Adresse
    Coroutine* coroutine = coroutine_new(stack, malloc(sizeof(int)));
    
    if (coroutine == NULL) {
        free(stack);
        return 1; // Ausfallfall: Nicht genug Speicher
    }
    
    // Füge eine Funktion zur Coroutine hinzu
    int my_function(void* context) {
        int* param = (int*)context;
        *param = *param + 1;
        printf("Parameter: %d\n", *param);
    }
    
    coroutine_add_function(coroutine, my_function);
    
    // Ausführe die Coroutine
    while (coroutine->state != -1) {
        switch (coroutine->state) {
            case 0:
                coroutine->state = 2; // Warte auf signalen
                break;
            case 1:
                coroutine->state = 0; // Laufend
                break;
            case 2: // Warte auf signalen
                if (/* Signal erhalten */) {
                    coroutine->state = 1; // Laufend
                } else {
                    coroutine->state = -1; // Abgeschlossen
                }
                break;
        }
    }
    
    free(coroutine);
    return 0;
}
