#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void* addr;
    size_t size;
} Leak;

typedef struct {
    void* addr;
    time_t timestamp;
} UseAfterFree;

typedef struct {
    void* addr;
    size_t size;
} Overflow;

class MemoryDebugger {
public:
    bool detect_leak(void* addr, size_t size) {
        if (leaks_ == nullptr) {
            leaks_ = malloc(sizeof(Leak));
            memset(leaks_, 0, sizeof(Leak));
        }
        leaks_[size] = {addr, size};
        return true;
    }

    bool detect_use_after_free(void* addr) {
        if (use_after_free_ == nullptr) {
            use_after_free_ = malloc(sizeof(UseAfterFree));
            memset(use_after_free_, 0, sizeof(UseAfterFree));
        }
        use_after_free_[time(NULL)] = {addr, time(NULL)};
        return true;
    }

    bool detect_overflow(void* addr, size_t size) {
        if (overflow_ == nullptr) {
            overflow_ = malloc(sizeof(Overflow));
            memset(overflow_, 0, sizeof(Overflow));
        }
        overflow_[size] = {addr, size};
        return true;
    }

    void print_stack_trace() {
        printf("Stack trace:\n");
        // Hier kommt der Code für die Ausgabe des Stack traces
        // zum Beispiel eine rekursive Funktion
        for (int i = 0; i < 10; i++) {
            printf("%d\n", i);
        }
    }

    void report() {
        printf("Bericht:\n");

        if (leaks_ != nullptr) {
            printf("Freigabe-Identifizierung:\n");
            for (size_t i = 0; i < leaks_->size; i++) {
                Leak leak = leaks_[i];
                printf("%p %zu\n", leak.addr, leak.size);
            }
        }

        if (use_after_free_ != nullptr) {
            printf("Nachbenutzung von Speicher:\n");
            for (time_t timestamp = time(NULL); timestamp > use_after_free_->timestamp; 
timestamp--) {
                UseAfterFree use_after_free = *use_after_free_;
                printf("%p %s\n", use_after_free.addr, ctime(&timestamp));
            }
        }

        if (overflow_ != nullptr) {
            printf("Buffer-Überschreitung-Schlechter:\n");
            for (size_t i = 0; i < overflow_->size; i++) {
                Overflow overflow = overflow_[i];
                printf("%p %zu\n", overflow.addr, overflow.size);
            }
        }

        print_stack_trace();
    }

private:
    void* leaks_;
    UseAfterFree* use_after_free_;
    Overflow* overflow_;
};

int main() {
    MemoryDebugger debugger;

    // Freigabe-Identifizierung
    void* addr1 = malloc(10);
    printf("Freigabe-Identifizierung:\n");
    if (debugger.detect_leak(addr1, 10)) {
        printf("%p %zu\n", addr1, 10);
    }

    // Nachbenutzung von Speicher
    void* addr2 = malloc(20);
    printf("Nachbenutzung von Speicher:\n");
    debugger.detect_use_after_free(addr2);

    // Buffer-Überschreitung-Schlechter
    char buffer[30];
    strcpy(buffer, "Überfüllter Buffer");
    printf("Buffer-Überschreitung-Schlechter:\n");
    if (debugger.detect_overflow(buffer, 30)) {
        printf("%s\n", buffer);
    }

    // Berichterstattung
    debugger.report();

    return 0;
}