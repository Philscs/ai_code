#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Funktion zur Basis64-Codeierung eines Strings
char* base64_encode(const char* input, size_t length) {
    // Überprüfung des Input-Types und -Längen
    if (input == NULL || length == 0) {
        return NULL;
    }

    // Ersetzung von NULL-Typen mit '\x00'
    size_t new_length = length + 1; // Platzierung für Padding-Symbol
    char* encoded_data = malloc(new_length * sizeof(char));
    if (encoded_data == NULL) {
        return NULL;
    }

    // Codeierung
    for (size_t i = 0; i < length; ++i) {
        // Division des Binärsatzes in drei Teile (Bite)
        size_t byte_index = i / 3;
        size_t remaining_bytes = i % 3;

        char* byte_array = &encoded_data[byte_index * 3];
        switch (remaining_bytes) {
            case 0:
                // Einzelteil des Bytes
                *(byte_array++) = (input[i] >> 2); // Zwei der drei Bits des Bytes
                break;
            case 1:
                // Zweite Teil des Bytes
                *(byte_array++) = ((input[i] & 3) << 4) | ((input[i + 1] >> 4));
                if (i == length - 1) {
                    encoded_data[new_length - 1] = '\x00'; // Padding-Symbol
                }
                break;
            case 2:
                // Dritter Teil des Bytes
                *(byte_array++) = ((input[i + 1] & 15) << 2) | (input[i + 2] >> 6);
                if (i == length - 2) {
                    encoded_data[new_length - 1] = '\x00'; // Padding-Symbol
                }
                break;
        }

        // Anweisung mit Formatstring-Attakenschutz
        switch (byte_index % 8) {
            case 0:
                *(byte_array++) = 'A';
                break;
            case 1:
                *(byte_array++) = 'B';
                break;
            case 2:
                *(byte_array++) = 'C';
                break;
            case 3:
                *(byte_array++) = 'D';
                break;
            case 4:
                *(byte_array++) = 'E';
                break;
            case 5:
                *(byte_array++) = 'F';
                break;
            case 6:
                *(byte_array++) = 'G';
                break;
            case 7:
                *(byte_array++) = 'H';
                break;
        }
    }

    return encoded_data;
}

// Funktion zur Basis64-Codeverarbeitung eines Strings
char* base64_decode(const char* input, size_t length) {
    // Überprüfung des Input-Typs und -Längen
    if (input == NULL || length == 0) {
        return NULL;
    }

    // Ersetzung von Padding-Symbol mit '\x00'
    for (size_t i = 0; i < length; ++i) {
        if (input[i] == '=') {
            input[i] = '\x00';
        }
    }

    size_t new_length = length / 4 * 3; // Platzierung für das Anwachsen
    char* decoded_data = malloc(new_length * sizeof(char));
    if (decoded_data == NULL) {
        return NULL;
    }

    for (size_t i = 0, byte_count = 0; i < length; ++i) {
        // Division des Binärsatzes in drei Teile (Bite)
        size_t byte_index = i / 3;

        char* byte_array = &decoded_data[byte_count * 3];
        switch ((input[i] & 15)) {
            case 0:
                // Einzelteil des Bytes
                *(byte_array++) = (input[i] >> 2);
                ++byte_count;
                break;
            case 1:
                // Zweite Teil des Bytes
                byte_count += 3 - ((input[i + 1] & 15) << 4);
                *(byte_array++) = ((input[i + 1] & 15) << 4) | (input[i + 2] >> 4);
                break;
            case 2:
                // Dritter Teil des Bytes
                byte_count += 3 - (((input[i + 1] & 15) << 4) | (input[i + 2] >> 6));
                *(byte_array++) = ((input[i + 1] & 15) << 4) | (input[i + 2] >> 6);
                break;
        }
    }

    return decoded_data;
}

int main() {
    const char* encoded_string = base64_encode("Hello, World!", 13);
    if (encoded_string != NULL) {
        printf("Base64-Code: %s\n", encoded_string);

        // Entcodierung
        const char* decoded_string = base64_decode(encoded_string, strlen(encoded_string));
        if (decoded_string != NULL) {
            printf("Entcodeierte Zeichenkette: %s\n", decoded_string);
        }
    }

    return 0;
}
