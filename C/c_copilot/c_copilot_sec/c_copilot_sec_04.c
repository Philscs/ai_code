#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} safe_string;

safe_string* create_safe_string(const char* str) {
    safe_string* s = malloc(sizeof(safe_string));
    if (s == NULL) {
        return NULL;
    }
    
    s->length = strlen(str);
    s->capacity = s->length + 1;
    s->data = malloc(s->capacity);
    if (s->data == NULL) {
        free(s);
        return NULL;
    }
    
    strncpy(s->data, str, s->capacity);
    return s;
}

void destroy_safe_string(safe_string* s) {
    if (s != NULL) {
        free(s->data);
        free(s);
    }
}

void safe_string_concat(safe_string* s1, const safe_string* s2) {
    size_t new_length = s1->length + s2->length;
    if (new_length >= s1->capacity) {
        size_t new_capacity = new_length + 1;
        char* new_data = realloc(s1->data, new_capacity);
        if (new_data == NULL) {
            // Handle memory allocation error
            return;
        }
        s1->data = new_data;
        s1->capacity = new_capacity;
    }
    
    strncat(s1->data, s2->data, s2->length);
    s1->length = new_length;
}

safe_string* safe_string_substring(const safe_string* s, size_t start, size_t length) {
    if (start >= s->length) {
        return NULL;
    }
    
    if (start + length > s->length) {
        length = s->length - start;
    }
    
    safe_string* substring = malloc(sizeof(safe_string));
    if (substring == NULL) {
        return NULL;
    }
    
    substring->length = length;
    substring->capacity = length + 1;
    substring->data = malloc(substring->capacity);
    if (substring->data == NULL) {
        free(substring);
        return NULL;
    }
    
    strncpy(substring->data, s->data + start, length);
    substring->data[length] = '\0';
    
    return substring;
}

int main() {
    safe_string* s1 = create_safe_string("Hello");
    safe_string* s2 = create_safe_string(" World!");
    
    safe_string_concat(s1, s2);
    printf("%s\n", s1->data);
    
    safe_string* s3 = safe_string_substring(s1, 6, 5);
    printf("%s\n", s3->data);
    
    destroy_safe_string(s1);
    destroy_safe_string(s2);
    destroy_safe_string(s3);
    
    return 0;
}
