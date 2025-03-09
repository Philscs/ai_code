#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} safe_string;

// Initialisiert einen neuen safe_string
safe_string* safe_string_create(const char* initial_data) {
    safe_string* str = (safe_string*)malloc(sizeof(safe_string));
    if (!str) return NULL;

    size_t initial_length = initial_data ? strlen(initial_data) : 0;
    size_t initial_capacity = initial_length + 1; // +1 für Nullterminator

    str->data = (char*)malloc(initial_capacity);
    if (!str->data) {
        free(str);
        return NULL;
    }

    if (initial_data) {
        memcpy(str->data, initial_data, initial_length);
    }
    str->data[initial_length] = '\0';
    str->length = initial_length;
    str->capacity = initial_capacity;

    return str;
}

// Gibt den Speicher frei
void safe_string_destroy(safe_string* str) {
    if (str) {
        free(str->data);
        free(str);
    }
}

// Überprüft, ob genug Kapazität vorhanden ist und vergrößert bei Bedarf
bool safe_string_ensure_capacity(safe_string* str, size_t needed_capacity) {
    if (!str || needed_capacity > SIZE_MAX - 1) return false;

    if (needed_capacity <= str->capacity) return true;

    // Verdoppele die Kapazität, bis sie ausreicht
    size_t new_capacity = str->capacity;
    while (new_capacity < needed_capacity) {
        if (new_capacity > SIZE_MAX / 2) return false; // Overflow-Check
        new_capacity *= 2;
    }

    char* new_data = (char*)realloc(str->data, new_capacity);
    if (!new_data) return false;

    str->data = new_data;
    str->capacity = new_capacity;
    return true;
}

// Konkateniert zwei Strings
bool safe_string_concat(safe_string* dest, const safe_string* src) {
    if (!dest || !src) return false;

    size_t new_length = dest->length + src->length;
    if (new_length < dest->length) return false; // Overflow-Check

    if (!safe_string_ensure_capacity(dest, new_length + 1)) return false;

    memcpy(dest->data + dest->length, src->data, src->length);
    dest->length = new_length;
    dest->data[dest->length] = '\0';

    return true;
}

// Erstellt einen Substring
safe_string* safe_string_substring(const safe_string* str, size_t start, size_t length) {
    if (!str || start > str->length) return NULL;

    // Begrenze die Länge auf verfügbare Zeichen
    if (length > str->length - start) {
        length = str->length - start;
    }

    safe_string* result = (safe_string*)malloc(sizeof(safe_string));
    if (!result) return NULL;

    result->data = (char*)malloc(length + 1);
    if (!result->data) {
        free(result);
        return NULL;
    }

    memcpy(result->data, str->data + start, length);
    result->data[length] = '\0';
    result->length = length;
    result->capacity = length + 1;

    return result;
}

// Kopiert einen String
safe_string* safe_string_copy(const safe_string* str) {
    if (!str) return NULL;
    return safe_string_substring(str, 0, str->length);
}

// Vergleicht zwei Strings
int safe_string_compare(const safe_string* str1, const safe_string* str2) {
    if (!str1 || !str2) return -2; // Fehlercode für ungültige Parameter

    size_t min_length = str1->length < str2->length ? str1->length : str2->length;
    int result = memcmp(str1->data, str2->data, min_length);
    
    if (result != 0) return result;
    if (str1->length < str2->length) return -1;
    if (str1->length > str2->length) return 1;
    return 0;
}

// Findet die Position eines Substrings
size_t safe_string_find(const safe_string* haystack, const safe_string* needle) {
    if (!haystack || !needle || needle->length > haystack->length) {
        return SIZE_MAX; // Nicht gefunden
    }

    for (size_t i = 0; i <= haystack->length - needle->length; i++) {
        if (memcmp(haystack->data + i, needle->data, needle->length) == 0) {
            return i;
        }
    }

    return SIZE_MAX; // Nicht gefunden
}

// Beispiel für die Verwendung
void example_usage() {
    // Erstelle einen neuen String
    safe_string* str1 = safe_string_create("Hello ");
    safe_string* str2 = safe_string_create("World!");
    
    if (str1 && str2) {
        // Konkateniere die Strings
        safe_string_concat(str1, str2);
        printf("Concatenated: %s\n", str1->data);
        
        // Erstelle einen Substring
        safe_string* sub = safe_string_substring(str1, 0, 5);
        if (sub) {
            printf("Substring: %s\n", sub->data);
            safe_string_destroy(sub);
        }
        
        // Cleanup
        safe_string_destroy(str1);
        safe_string_destroy(str2);
    }
}