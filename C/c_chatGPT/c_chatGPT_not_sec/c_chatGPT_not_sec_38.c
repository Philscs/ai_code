#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE2

#define WIDTH 1024
#define HEIGHT 768

void process_image_sse(unsigned char *input, unsigned char *output, int width, int height) {
    int num_pixels = width * height;

    // Prozessieren Sie 16 Pixel gleichzeitig (4 Kanäle: RGBA * 4 = 16 Bytes pro Operation)
    for (int i = 0; i < num_pixels * 4; i += 16) {
        // Laden von 16 Bytes aus dem Eingabebild
        __m128i pixels = _mm_loadu_si128((__m128i *)(input + i));

        // Beispieloperation: Helligkeit um einen festen Wert erhöhen (hier: 50)
        __m128i brightness = _mm_set1_epi8(50); // Alle 16 Werte auf 50 setzen
        __m128i result = _mm_adds_epu8(pixels, brightness); // Saturierte Addition

        // Ergebnis in das Ausgabebild speichern
        _mm_storeu_si128((__m128i *)(output + i), result);
    }
}

int main() {
    // Speicher für das Eingabe- und Ausgabebild
    unsigned char *input_image = (unsigned char *)aligned_alloc(16, WIDTH * HEIGHT * 4);
    unsigned char *output_image = (unsigned char *)aligned_alloc(16, WIDTH * HEIGHT * 4);

    if (!input_image || !output_image) {
        fprintf(stderr, "Fehler: Speicherzuweisung fehlgeschlagen\n");
        return 1;
    }

    // Beispiel: Eingabebild initialisieren (hier zufällige Pixelwerte)
    for (int i = 0; i < WIDTH * HEIGHT * 4; i++) {
        input_image[i] = rand() % 256;
    }

    // Bild verarbeiten
    process_image_sse(input_image, output_image, WIDTH, HEIGHT);

    // Ergebnis speichern oder weiterverarbeiten (z.B. als Datei speichern)

    // Speicher freigeben
    free(input_image);
    free(output_image);

    return 0;
}
