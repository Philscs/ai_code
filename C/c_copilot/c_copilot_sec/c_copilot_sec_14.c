#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Input Validation
bool isBase64Char(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '+' || c == '/';
}

bool isValidBase64String(const char* str) {
    size_t len = strlen(str);
    if (len % 4 != 0) {
        return false; // Invalid length
    }

    for (size_t i = 0; i < len; i++) {
        if (!isBase64Char(str[i])) {
            return false; // Invalid character
        }
    }

    return true;
}

// Padding Validation
bool hasValidPadding(const char* str) {
    size_t len = strlen(str);
    if (len % 4 != 0) {
        return false; // Invalid length
    }

    int paddingCount = 0;
    for (int i = len - 1; i >= 0; i--) {
        if (str[i] == '=') {
            paddingCount++;
        } else {
            break;
        }
    }

    return paddingCount <= 2;
}

// Memory Safe Operations
void safeMemcpy(char* dest, const char* src, size_t destSize) {
    size_t srcLen = strlen(src);
    size_t copyLen = srcLen < destSize ? srcLen : destSize - 1;
    memcpy(dest, src, copyLen);
    dest[copyLen] = '\0';
}

// Base64 Encoder
char* base64Encode(const char* input) {
    if (!isValidBase64String(input)) {
        return NULL;
    }

    size_t inputLen = strlen(input);
    size_t outputLen = ((inputLen + 2) / 3) * 4 + 1;
    char* output = malloc(outputLen);
    if (output == NULL) {
        return NULL;
    }

    // Encoding logic goes here...

    return output;
}

// Base64 Decoder
char* base64Decode(const char* input) {
    if (!isValidBase64String(input) || !hasValidPadding(input)) {
        return NULL;
    }

    size_t inputLen = strlen(input);
    size_t outputLen = (inputLen / 4) * 3 + 1;
    char* output = malloc(outputLen);
    if (output == NULL) {
        return NULL;
    }

    // Decoding logic goes here...

    return output;
}

int main() {
    const char* input = "SGVsbG8gV29ybGQh"; // "Hello World!"
    char* encoded = base64Encode(input);
    if (encoded != NULL) {
        printf("Encoded: %s\n", encoded);
        free(encoded);
    }

    const char* encodedInput = "SGVsbG8gV29ybGQh"; // "Hello World!"
    char* decoded = base64Decode(encodedInput);
    if (decoded != NULL) {
        printf("Decoded: %s\n", decoded);
        free(decoded);
    }

    return 0;
}
