#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Helper function to validate input
bool is_valid_base64(const char *input) {
    size_t len = strlen(input);

    if (len % 4 != 0) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        if (!(
                (input[i] >= 'A' && input[i] <= 'Z') ||
                (input[i] >= 'a' && input[i] <= 'z') ||
                (input[i] >= '0' && input[i] <= '9') ||
                input[i] == '+' ||
                input[i] == '/' ||
                input[i] == '=')) {
            return false;
        }
    }

    return true;
}

// Base64 Encoding Function
char *base64_encode(const unsigned char *data, size_t len) {
    if (!data || len == 0) {
        return NULL;
    }

    size_t output_len = 4 * ((len + 2) / 3);
    char *encoded = (char *)calloc(output_len + 1, sizeof(char)); // +1 for null terminator

    if (!encoded) {
        return NULL;
    }

    for (size_t i = 0, j = 0; i < len;) {
        uint32_t octet_a = i < len ? data[i++] : 0;
        uint32_t octet_b = i < len ? data[i++] : 0;
        uint32_t octet_c = i < len ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded[j++] = i > len + 1 ? '=' : base64_table[(triple >> 6) & 0x3F];
        encoded[j++] = i > len ? '=' : base64_table[triple & 0x3F];
    }

    return encoded;
}

// Base64 Decoding Function
unsigned char *base64_decode(const char *input, size_t *out_len) {
    if (!input || !is_valid_base64(input)) {
        return NULL;
    }

    size_t len = strlen(input);
    size_t padding = (input[len - 1] == '=') + (input[len - 2] == '=');
    *out_len = (len / 4 * 3) - padding;

    unsigned char *decoded = (unsigned char *)calloc(*out_len, sizeof(unsigned char));
    if (!decoded) {
        return NULL;
    }

    for (size_t i = 0, j = 0; i < len;) {
        uint32_t sextet_a = input[i] == '=' ? 0 & i++ : strchr(base64_table, input[i++]) - base64_table;
        uint32_t sextet_b = input[i] == '=' ? 0 & i++ : strchr(base64_table, input[i++]) - base64_table;
        uint32_t sextet_c = input[i] == '=' ? 0 & i++ : strchr(base64_table, input[i++]) - base64_table;
        uint32_t sextet_d = input[i] == '=' ? 0 & i++ : strchr(base64_table, input[i++]) - base64_table;

        uint32_t triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        if (j < *out_len) decoded[j++] = (triple >> 16) & 0xFF;
        if (j < *out_len) decoded[j++] = (triple >> 8) & 0xFF;
        if (j < *out_len) decoded[j++] = triple & 0xFF;
    }

    return decoded;
}

// Main function for demonstration
int main() {
    const char *data = "SecureBase64";
    size_t encoded_len, decoded_len;

    // Encoding
    char *encoded = base64_encode((const unsigned char *)data, strlen(data));
    if (!encoded) {
        fprintf(stderr, "Encoding failed\n");
        return EXIT_FAILURE;
    }
    printf("Encoded: %s\n", encoded);

    // Decoding
    unsigned char *decoded = base64_decode(encoded, &decoded_len);
    if (!decoded) {
        fprintf(stderr, "Decoding failed\n");
        free(encoded);
        return EXIT_FAILURE;
    }

    printf("Decoded: %.*s\n", (int)decoded_len, decoded);

    free(encoded);
    free(decoded);

    return EXIT_SUCCESS;
}
