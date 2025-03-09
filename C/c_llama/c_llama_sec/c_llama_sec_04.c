#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} safe_string;

// Initialisierung einer neuen safe_string-Instanz
safe_string new_safe_string() {
    safe_string s;
    s.data = malloc(1); // Initialisiere mit einem Array von Länge 1
    if (s.data == NULL) {
        return (safe_string){0, 0, 0}; // Keine Speicherzuweisung erfolgreich
    }
    s.length = 0;
    s.capacity = 1; // Initialisierung der Kapazität auf 1
    return s;
}

// Erweitere die Kapazität des Strings und aktualisiere die Länge und Kapazität
void safe_string_ensure_capacity(safe_string* s, size_t new_capacity) {
    if (new_capacity <= s->capacity) {
        return; // Keine Änderung der Kapazität erforderlich
    }

    char* new_data = realloc(s->data, new_capacity); // Erweitere den Speicher mit realloc
    if (new_data == NULL) {
        return; // Keine Speicherzuweisung erfolgreich
    }
    s->data = new_data;
    s->capacity = new_capacity;
}

// Setze die Länge des Strings neu und aktualisiere die Kapazität wenn notwendig
void safe_string_set_length(safe_string* s, size_t length) {
    if (length < 0 || length > s->capacity) {
        return; // Keine Änderung der Länge erforderlich oder überschritten
    }
    s->length = length;
}

// Erzeugen einer neuen Substring-Instanz mit einem gegebenen Startpunkt und Ende
safe_string safe_string_substring(safe_string* s, size_t start, size_t end) {
    if (start < 0 || end > s->capacity || start >= end) {
        return (safe_string){NULL, 0, 0}; // Keine gültige Substring-Instanz
    }
    safe_string new_s;
    new_s.data = malloc(end - start); // Erstelle einen neuen String mit der gewünschten Länge
    if (new_s.data == NULL) {
        return (safe_string){NULL, 0, 0}; // Keine Speicherzuweisung erfolgreich
    }
    memcpy(new_s.data, s->data + start, end - start); // Kopiere den gewünschten Teil in den 
neuen String
    new_s.length = end - start;
    new_s.capacity = end - start; // Ermittle die Kapazität des neuen Strings
    return new_s;
}

// Concatenieren eines gegebenen Strings mit einem anderen gegebenen String
safe_string safe_string_concat(safe_string* s1, char* s2) {
    size_t total_length = s1->length + strlen(s2);
    if (total_length > s1->capacity) { // Wenn die Kapazität nicht genügt ist, erweitere sie
        safe_string_ensure_capacity(&s1->data, total_length + 1); // Addieren der 
Null-Endezeichen
    }
    memcpy(s1->data + s1->length, s2, strlen(s2)); // Kopiere den zweiten String in den ersten 
String
    s1->length += strlen(s2);
    return *s1; // Rückgabewert des geänderten Strings
}

// Auflösen eines gewünschten Teilstreangs aus einem anderen
char* safe_string_substr(safe_string* s, size_t start) {
    if (start < 0 || start >= s->length) {
        return NULL; // Keine gültige Substring-Instanz
    }
    char* new_data = malloc((s->length - start)); // Erstelle einen neuen String mit der 
gewünschten Länge
    if (new_data == NULL) {
        return NULL; // Keine Speicherzuweisung erfolgreich
    }
    memcpy(new_data, s->data + start, (s->length - start)); // Kopiere den gewünschten Teil in 
den neuen String
    return new_data;
}

// Löschen eines gegebenen Strings
void safe_string_destroy(safe_string* s) {
    free(s->data); // Lösche die Speicherzuge
    *s = (safe_string){NULL, 0, 0}; // Zerstelle den String, indem du die Daten auf NULL setzen.
}

// Beispiele für die Verwendung des safe-string-Handling:
int main() {
    // Erstellung eines neuen Strings
    safe_string s1;
    printf("%s\n", safe_string_substr(&s1, 0)); // Ausgabe: ""
    
    // Concatenieren eines gegebenen Strings mit einem anderen
    char s2[] = "Hallo";
    safe_string s3 = new_safe_string();
    s3 = safe_string_concat(&s1, s2);
    printf("%s\n", s3.data); // Ausgabe: "Hallo"

    // Substring-Instanz aus einem gegebenen String erstellen
    safe_string s4 = new_safe_string();
    s4 = safe_string_substring(&s1, 0, 5);
    printf("%s\n", s4.data); // Ausgabe: ""

    return 0;
}