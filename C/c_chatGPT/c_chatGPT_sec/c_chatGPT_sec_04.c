#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definition der safe_string Struktur
typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} safe_string;

// Hilfsfunktion: Sicherer Speicher
void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Initialisierung eines Strings
safe_string* string_init(size_t capacity) {
    safe_string* s = (safe_string*)safe_malloc(sizeof(safe_string));
    s->data = (char*)safe_malloc(capacity);
    s->data[0] = '\0';
    s->length = 0;
    s->capacity = capacity;
    return s;
}

// Freigabe des Strings
void string_free(safe_string* s) {
    if (s) {
        free(s->data);
        free(s);
    }
}

// Resizing des Strings
void string_resize(safe_string* s, size_t new_capacity) {
    if (new_capacity <= s->capacity) {
        return;
    }
    s->data = (char*)realloc(s->data, new_capacity);
    if (!s->data) {
        fprintf(stderr, "Memory allocation failed during resize\n");
        exit(EXIT_FAILURE);
    }
    s->capacity = new_capacity;
}

// String kopieren
void string_copy(safe_string* dest, const char* src) {
    size_t len = strlen(src);
    if (len + 1 > dest->capacity) {
        string_resize(dest, len + 1);
    }
    strcpy(dest->data, src);
    dest->length = len;
}

// String anhängen (concat)
void string_concat(safe_string* dest, const char* src) {
    size_t src_len = strlen(src);
    if (dest->length + src_len + 1 > dest->capacity) {
        string_resize(dest, dest->length + src_len + 1);
    }
    strcat(dest->data, src);
    dest->length += src_len;
}

// Substring extrahieren
safe_string* string_substring(const safe_string* s, size_t start, size_t length) {
    if (start >= s->length) {
        fprintf(stderr, "Start index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    if (start + length > s->length) {
        length = s->length - start;
    }
    safe_string* sub = string_init(length + 1);
    strncpy(sub->data, s->data + start, length);
    sub->data[length] = '\0';
    sub->length = length;
    return sub;
}

// Vergleich zweier Strings
int string_compare(const safe_string* s1, const safe_string* s2) {
    return strcmp(s1->data, s2->data);
}

// String zurücksetzen
void string_clear(safe_string* s) {
    s->data[0] = '\0';
    s->length = 0;
}

// Beispielnutzung
int main() {
    safe_string* s1 = string_init(10);
    string_copy(s1, "Hello");

    safe_string* s2 = string_init(10);
    string_copy(s2, " World");

    string_concat(s1, s2->data);
    printf("Concatenated String: %s\n", s1->data);

    safe_string* sub = string_substring(s1, 6, 5);
    printf("Substring: %s\n", sub->data);

    printf("Comparison: %d\n", string_compare(s1, s2));

    string_clear(s1);
    printf("Cleared String: %s\n", s1->data);

    string_free(s1);
    string_free(s2);
    string_free(sub);

    return 0;
}
