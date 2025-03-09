#include <stdio.h>
#include <stdlib.h>

// Strukt für die Welle (DCT)
typedef struct {
    int16_t **welle;
} dct_welle;

// Funktion zur Berechnung von Welle (DCT) in einem 2D-Array
void dct(dct_welle *welle, int h, int w) {
    int i, j;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            int16_t temp = welle->welle[i*w+j];
            int16_t sum = 0;
            for (int k = -1; k <= 1; k += 2) {
                for (int l = -1; l <= 1; l += 2) {
                    if ((i+k >= 0 && i+k < h) && (j+l >= 0 && j+l < w)) {
                        sum += welle->welle[(i+k)*w+j+l]*cos(3.14159265359*(k+1)*(l+1));
                    }
                }
            }
            welle->welle[i*w+j] = temp - sum;
        }
    }
}

// Funktion zum Erstellen der Huffman-Code-Liste
void create_huffman_list(dct_welle *welle, int h) {
    // Berechnung von Häufigkeiten
    int freq[256] = {0};
    for (int i = 0; i < welle->welle[h][h]; i++) {
        freq[(int)welle->welle[i][i]]++;
    }

    // Erstelle die Huffman-Code-Liste
    huffman_code(welle, freq);
}

// Funktion zum Codieren eines Bildes mit JPEG-Format
void jpeg_encode(dct_welle *welle, int h, FILE *out_file) {
    create_huffman_list(welle, h);

    // Speichere den JPEG-Header
    fwrite("FFD8", sizeof(char), 1, out_file);
    fwrite("FFE0", sizeof(char), 1, out_file);

    // Iteriere über die Welle und codiere sie mit Huffman-Kode
    for (int i = 0; i < h*h; i++) {
        int temp = 0;
        for (int j = 0; welle->welle[i][j] != 0; j += 1) {
            if ((welle->welle[i][j] & 0x80) == 0) {
                // Kein Bit gesetzt
                temp <<= 1;
            } else {
                // Setzen eines Bits
                temp |= (welle->welle[i][j] & 0x01);
                welle->welle[i][j] >>= 1;
            }
        }

        if (temp >= 0) {
            fwrite(&temp, sizeof(int), 1, out_file);
        }
    }
}

// Funktion zum Decodieren eines Bildes mit JPEG-Format
void jpeg_decode(char *binary_data, int h, FILE *in_file, dct_welle *welle) {
    // Dekodiere den JPEG-Header
    if (h == 0 || welle->welle[h][h] == 0) {
        printf("Fehler beim Decodieren des JPEG-Bildes");
        return;
    }

    // Iteriere über die Welle und decodiere sie mit Huffman-Kode
    for (int i = 0; i < h*h; i++) {
        int temp = 0;
        for (int j = 0; binary_data[i*h+j] != 0; j += 1) {
            if ((binary_data[i*h+j] & 0x80) == 0) {
                // Kein Bit gesetzt
                temp <<= 1;
            } else {
                // Setzen eines Bits
                temp |= (binary_data[i*h+j] & 0x01);
                binary_data[i*h+j] >>= 1;
            }
        }

        if (temp >= 0) {
            welle->welle[i][j] = temp;
        }
    }

    huffman_decode(welle, h, h, binary_data);
}

// Funktion zum Decodieren der Huffman-Kode-Liste
void huffman_decode(dct_welle *welle, int h, FILE *in_file) {
    // Dekodiere den JPEG-Header
    if (h == 0 || welle->welle[h][h] == 0) {
        printf("Fehler beim Decodieren des JPEG-Bildes");
        return;
    }

    // Iteriere über die Welle und decodiere sie mit Huffman-Kode
    for (int i = 0; i < h*h; i++) {
        int temp = welle->welle[i][i];
        if (temp >= 0) {
            fwrite(&temp, sizeof(int), 1, in_file);
        }
    }
}

// Funktion zum Generieren der binären Form des Huffman-Kodes
void huffman_code(dct_welle *welle, int freq[256], char *binary_data) {
    // Erstelle die Prioritäten-Liste
    int prior[256] = {0};
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            prior[i] = freq[i];
        }
    }

    // Erstelle die Huffman-Kode-Liste
    char *node_list[128] = {0};
    int leaf_count = 0;
    for (int i = 1; i <= 127; i++) {
        node_list[i] = malloc(sizeof(char));
        for (int j = 0; prior[j] != 0; j += 1) {
            if (prior[j] == 0 || prior[j+1] == 0) {
                leaf_count++;
            }
        }

        // Setze den Code für das ursprüngliche Ziel
        if (leaf_count > 0) {
            strcpy(node_list[i], "0");
        } else {
            node_list[i] = "1";
        }
    }

    // Generiere die binäre Form des Huffman-Kodes
    for (int i = 128; i <= 255; i++) {
        char temp[256] = {0};
        int j = i;
        while (j > 0) {
            if ((j & 1) == 1) {
                strcpy(temp, node_list[j+1]);
            }
            strcat(temp, "0");
            j >>= 1;
        }

        // Speichere die binäre Form des Huffman-Kodes
        for (int k = 0; temp[k] != '\0'; k += 1) {
            binary_data[leaf_count+j] = temp[k];
            leaf_count++;
        }
    }
}

// Funktion zum Generieren der Prioritäten-Liste für den Huffman-Kode
void huffman_prior(dct_welle *welle, int freq[256]) {
    // Erstelle die Prioritäten-Liste
    for (int i = 1; i <= 127; i++) {
        if (freq[i] > 0) {
            prior[i] = freq[i];
        }
    }

    // Sortiere die Prioritäten-Liste
    qsort(prior, 128, sizeof(int), compare);
}

// Funktion zum Vergleichen zweier Werte in der Prioritäten-Liste
int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

int main() {
    // Erstelle die Welle mit den Werten
    dct_welle welle;
    for (int i = 0; i < 256; i++) {
        welle.welle[i][i] = rand();
    }

    int freq[256] = {0};
    huffman_prior(&welle, freq);

    // Generiere die binäre Form des Huffman-Kodes
    char binary_data[128*100];
    huffman_code(&welle, freq, binary_data);

    // Speichere die Welle und die Prioritäten-Liste
    FILE *out_file = fopen("welle.bin", "wb");
    fwrite(&welle.welle[0][0], sizeof(int), welle.welle[0][0], out_file);
    fclose(out_file);

    FILE *in_file = fopen("prioritize.bin", "rb");
    fread(welle.welle[0][0], sizeof(int), welle.welle[0][0], in_file);
    fclose(in_file);

    // Speichere die binäre Form des Huffman-Kodes
    FILE *bin_out_file = fopen("huffman_code.bin", "wb");
    fwrite(binary_data, sizeof(char), 128*100, bin_out_file);
    fclose(bin_out_file);

    // Decodieren Sie das Bild
    char image[128*100];
    fread(image, sizeof(char), 128*100, in_file);
    fclose(in_file);

    jpeg_decode(image, 256, out_file, &welle);
    fclose(out_file);

    return 0;
}
