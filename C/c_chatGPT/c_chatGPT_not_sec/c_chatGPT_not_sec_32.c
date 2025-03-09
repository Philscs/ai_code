#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// Prototypen
void compile_and_execute(const char *expression);

int main() {
    const char *expression = "3 + 5 - 2"; // Beispielausdruck
    compile_and_execute(expression);
    return 0;
}

// Funktion zum Übersetzen und Ausführen eines arithmetischen Ausdrucks
void compile_and_execute(const char *expression) {
    // Speicher für den Maschinencode allozieren
    unsigned char *code = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    unsigned char *p = code;

    // x86-64 Prolog: Funktionseintritt
    *p++ = 0x55;             // push rbp
    *p++ = 0x48; *p++ = 0x89; *p++ = 0xe5; // mov rbp, rsp

    // RAX = 0 (Initialwert)
    *p++ = 0x48; *p++ = 0xc7; *p++ = 0xc0; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00; // mov rax, 0

    const char *current = expression;
    while (*current) {
        if (*current >= '0' && *current <= '9') {
            // Zahl in RAX laden
            int value = strtol(current, (char **)&current, 10);
            *p++ = 0x48; *p++ = 0xc7; *p++ = 0xc1; // mov rcx, value
            memcpy(p, &value, 4);
            p += 4;

            // Zahl addieren/subtrahieren basierend auf dem Operator
            if (*(current - 1) == '+') {
                *p++ = 0x48; *p++ = 0x01; *p++ = 0xc8; // add rax, rcx
            } else if (*(current - 1) == '-') {
                *p++ = 0x48; *p++ = 0x29; *p++ = 0xc8; // sub rax, rcx
            }
        } else {
            current++; // Weiter zum nächsten Zeichen
        }
    }

    // x86-64 Epilog: Funktion beenden
    *p++ = 0x5d;             // pop rbp
    *p++ = 0xc3;             // ret

    // Funktion ausführen
    int (*func)() = (int (*)())code;
    int result = func();

    printf("Das Ergebnis von '%s' ist: %d\n", expression, result);

    // Speicher freigeben
    munmap(code, 4096);
}
