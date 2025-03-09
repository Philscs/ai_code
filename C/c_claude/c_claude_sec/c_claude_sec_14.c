#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#define BASE64_PAD '='
#define MAX_INPUT_LENGTH (SIZE_MAX / 4 * 3)  // Prevent integer overflow
#define MIN_INPUT_LENGTH 1

// Base64 Alphabet
static const char base64_charset[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Lookup table für schnelles Dekodieren
static const int base64_decode_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0-15
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 16-31
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63, // 32-47
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1, // 48-63
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, // 64-79
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1, // 80-95
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, // 96-111
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1, // 112-127
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 128-143
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 144-159
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 160-175
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 176-191
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 192-207
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 208-223
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 224-239
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1  // 240-255
};

// Sicheres Strukturformat für Ein- und Ausgabepuffer
typedef struct {
    unsigned char* data;
    size_t length;
} secure_buffer_t;

// Validierung der Eingabelänge
static bool validate_input_length(size_t input_length, bool is_encoding) {
    if (input_length < MIN_INPUT_LENGTH || input_length > MAX_INPUT_LENGTH) {
        return false;
    }
    
    if (!is_encoding) {
        // Für Dekodierung: Länge muss durch 4 teilbar sein
        if (input_length % 4 != 0) {
            return false;
        }
    }
    
    return true;
}

// Validierung des Base64-Paddings
static bool validate_padding(const unsigned char* input, size_t length) {
    if (length < 2) return false;
    
    // Prüfe Position und Anzahl der Padding-Zeichen
    size_t padding_count = 0;
    for (size_t i = length - 1; i >= length - 2; i--) {
        if (input[i] == BASE64_PAD) {
            padding_count++;
        } else {
            break;
        }
    }
    
    // Validiere Padding-Regeln
    if (padding_count > 2) return false;
    if (padding_count == 1 && input[length-2] != BASE64_PAD) return false;
    
    return true;
}

// Base64 Encoding
bool base64_encode(const secure_buffer_t* input, secure_buffer_t* output) {
    if (!input || !output || !input->data) {
        return false;
    }
    
    // Validiere Eingabelänge
    if (!validate_input_length(input->length, true)) {
        return false;
    }
    
    // Berechne benötigte Ausgabegröße
    size_t output_len = 4 * ((input->length + 2) / 3);
    
    // Alloziere Ausgabepuffer
    output->data = (unsigned char*)calloc(output_len + 1, sizeof(unsigned char));
    if (!output->data) {
        return false;
    }
    output->length = output_len;
    
    // Encoding-Prozess
    const unsigned char* in = input->data;
    unsigned char* out = output->data;
    size_t i = 0, j = 0;
    
    while (i < input->length) {
        uint32_t octet_a = i < input->length ? in[i++] : 0;
        uint32_t octet_b = i < input->length ? in[i++] : 0;
        uint32_t octet_c = i < input->length ? in[i++] : 0;
        
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        out[j++] = base64_charset[(triple >> 18) & 0x3F];
        out[j++] = base64_charset[(triple >> 12) & 0x3F];
        out[j++] = base64_charset[(triple >> 6) & 0x3F];
        out[j++] = base64_charset[triple & 0x3F];
    }
    
    // Füge Padding hinzu
    if (input->length % 3 > 0) {
        size_t pad_count = 3 - (input->length % 3);
        for (size_t i = 0; i < pad_count; i++) {
            output->data[output_len - 1 - i] = BASE64_PAD;
        }
    }
    
    output->data[output_len] = '\0';
    return true;
}

// Base64 Decoding
bool base64_decode(const secure_buffer_t* input, secure_buffer_t* output) {
    if (!input || !output || !input->data) {
        return false;
    }
    
    // Validiere Eingabelänge und Padding
    if (!validate_input_length(input->length, false) || 
        !validate_padding(input->data, input->length)) {
        return false;
    }
    
    // Berechne Ausgabegröße
    size_t padding_count = 0;
    if (input->length >= 2) {
        if (input->data[input->length-1] == BASE64_PAD) padding_count++;
        if (input->data[input->length-2] == BASE64_PAD) padding_count++;
    }
    
    size_t output_len = (input->length / 4) * 3 - padding_count;
    
    // Alloziere Ausgabepuffer
    output->data = (unsigned char*)calloc(output_len + 1, sizeof(unsigned char));
    if (!output->data) {
        return false;
    }
    output->length = output_len;
    
    // Decoding-Prozess
    const unsigned char* in = input->data;
    unsigned char* out = output->data;
    size_t i = 0, j = 0;
    
    while (i < input->length - padding_count) {
        // Validiere Zeichen
        if (base64_decode_table[in[i]] == -1 || 
            base64_decode_table[in[i+1]] == -1 || 
            base64_decode_table[in[i+2]] == -1 || 
            base64_decode_table[in[i+3]] == -1) {
            free(output->data);
            output->data = NULL;
            return false;
        }
        
        uint32_t sextet_a = base64_decode_table[in[i]];
        uint32_t sextet_b = base64_decode_table[in[i+1]];
        uint32_t sextet_c = base64_decode_table[in[i+2]];
        uint32_t sextet_d = base64_decode_table[in[i+3]];
        
        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + 
                         (sextet_c << 6) + sextet_d;
        
        if (j < output_len) out[j++] = (triple >> 16) & 0xFF;
        if (j < output_len) out[j++] = (triple >> 8) & 0xFF;
        if (j < output_len) out[j++] = triple & 0xFF;
        
        i += 4;
    }
    
    output->data[output_len] = '\0';
    return true;
}

// Beispiel für die Verwendung
void example_usage(void) {
    const char* test_string = "Hello, World!";
    
    // Eingabepuffer erstellen
    secure_buffer_t input = {
        .data = (unsigned char*)test_string,
        .length = strlen(test_string)
    };
    
    // Ausgabepuffer für Encoding
    secure_buffer_t encoded = {0};
    
    // Encode
    if (base64_encode(&input, &encoded)) {
        printf("Encoded: %s\n", encoded.data);
        
        // Ausgabepuffer für Decoding
        secure_buffer_t decoded = {0};
        
        // Decode
        if (base64_decode(&encoded, &decoded)) {
            printf("Decoded: %s\n", decoded.data);
            free(decoded.data);
        }
        
        free(encoded.data);
    }
}