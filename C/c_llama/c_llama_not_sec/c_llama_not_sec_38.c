#include <stdio.h>
#include <stdint.h>

// Funktionsweise der Sobel-Operator für horizontale Richtung
void sobel_horizontal(uint16_t *image, int width, int height, uint16_t *output) {
    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x += 2) {
            // Liniennetzrate
            output[0] = image[y * width + x + 1] + image[y * width + x];
            output[0] >>= 8;
        }
    }
}

// Funktionsweise der Sobel-Operator für vertikale Richtung
void sobel_vertical(uint16_t *image, int width, int height, uint16_t *output) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x += 2) {
            // Spaltenladierte horizontale Richtung
            output[0] = image[(y + 1) * width + x] + image[(y - 1) * width + x];
            output[0] >>= 8;
        }
    }

    for (int y = 0; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x += 2) {
            // Spaltenladierte horizontale Richtung
            output[1] = image[y * width + x + 1] + image[y * width + x];
            output[1] >>= 8;
        }
    }
}

// Funktionsweise der Sobel-Operator für diagonal Richtung (hoch links)
void sobel_diagonal_hoch_links(uint16_t *image, int width, int height, uint16_t *output) {
    for (int y = 1; y < height - 2; y++) {
        for (int x = 1; x < width - 2; x += 2) {
            // Diagonale Richtung hoch links
            output[0] = image[(y + 1) * width + (x + 1)] + image[(y - 1) * width + (x - 1)];
            output[0] >>= 8;
        }
    }

    for (int y = 2; y < height - 1; y++) {
        for (int x = 1; x < width - 2; x += 2) {
            // Diagonale Richtung hoch links
            output[1] = image[y * width + x + 1] + image[(y + 1) * width + x];
            output[1] >>= 8;
        }
    }

    for (int y = 0; y < height - 2; y++) {
        for (int x = 2; x < width - 3; x += 2) {
            // Diagonale Richtung hoch links
            output[2] = image[(y + 1) * width + (x + 1)] + image[(y - 1) * width + (x - 1)];
            output[2] >>= 8;
        }
    }

    for (int y = 0; y < height - 2; y++) {
        for (int x = 3; x < width - 3; x += 2) {
            // Diagonale Richtung hoch links
            output[3] = image[y * width + x + 1] + image[(y + 1) * width + x];
            output[3] >>= 8;
        }
    }
}

// Funktionsweise der Sobel-Operator für diagonal Richtung (hoch rechts)
void sobel_diagonal_hoch_recht(uint16_t *image, int width, int height, uint16_t *output) {
    for (int y = 1; y < height - 2; y++) {
        for (int x = 3; x < width - 3; x += 2) {
            // Diagonale Richtung hoch rechts
            output[0] = image[(y + 1) * width + (x + 1)] + image[(y - 1) * width + (x - 1)];
            output[0] >>= 8;
        }
    }

    for (int y = 2; y < height - 1; y++) {
        for (int x = 3; x < width - 3; x += 2) {
            // Diagonale Richtung hoch rechts
            output[1] = image[y * width + x + 1] + image[(y + 1) * width + x];
            output[1] >>= 8;
        }
    }

    for (int y = 0; y < height - 2; y++) {
        for (int x = 5; x < width - 3; x += 2) {
            // Diagonale Richtung hoch rechts
            output[2] = image[(y + 1) * width + (x + 1)] + image[(y - 1) * width + (x - 1)];
            output[2] >>= 8;
        }
    }

    for (int y = 0; y < height - 2; y++) {
        for (int x = 7; x < width - 3; x += 2) {
            // Diagonale Richtung hoch rechts
            output[3] = image[y * width + x + 1] + image[(y + 1) * width + x];
            output[3] >>= 8;
        }
    }
}

int main() {
    const int width = 256;
    const int height = 256;

    uint16_t image[width * height];
    uint16_t output_h[width][height / 2];

    // Füllen der Image mit Randwerten
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            image[y * width + x] = rand() % 256;
        }
    }

    // Durchführung der Sobel-Operator für horizontale Richtung
    sobel_horizontal(image, width, height, output_h[0]);

    // Durchführung der Sobel-Operator für vertikale Richtung
    sobel_vertical(image, width, height, output_h[1]);

    // Durchführung der Sobel-Operator für diagonal Richtung (hoch links)
    sobel_diagonal_hoch_links(image, width, height, output_h[2] + (width / 4));

    // Durchführung der Sobel-Operator für diagonal Richtung (hoch rechts)
    sobel_diagonal_hoch_recht(image, width, height, output_h[3] + (width / 4));

    // Ausgabe der Ergebnisse
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            printf("%02x ", output_h[y * 2][x]);
        }
        printf("\n");
    }

    return 0;
}
