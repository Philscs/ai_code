#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>
#include <string.h>

// Struktur für ein Bild
typedef struct {
    unsigned char* data;
    int width;
    int height;
    int channels;  // 1 für Graustufen, 3 für RGB, 4 für RGBA
} Image;

// Hilfsfunktion zum Erstellen eines Bildes
Image* create_image(int width, int height, int channels) {
    Image* img = (Image*)malloc(sizeof(Image));
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = (unsigned char*)aligned_alloc(32, width * height * channels * sizeof(unsigned char));
    return img;
}

// Hilfsfunktion zum Freigeben des Bildspeichers
void free_image(Image* img) {
    free(img->data);
    free(img);
}

// SIMD-optimierte Helligkeitsanpassung
void adjust_brightness_simd(Image* img, float factor) {
    int total_pixels = img->width * img->height * img->channels;
    int simd_width = 8;  // AVX2 verarbeitet 8 Floats gleichzeitig
    
    __m256 factor_vec = _mm256_set1_ps(factor);
    
    // Verarbeite Pixel in 8er-Blöcken
    for (int i = 0; i < total_pixels - simd_width + 1; i += simd_width) {
        // Lade 8 Pixel
        __m256i pixels = _mm256_loadu_si256((__m256i*)&img->data[i]);
        // Konvertiere zu Float
        __m256 pixels_float = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_loadu_si128((__m128i*)&img->data[i])));
        // Multipliziere mit Faktor
        __m256 adjusted = _mm256_mul_ps(pixels_float, factor_vec);
        // Konvertiere zurück zu Integer und klemme auf 0-255
        __m256i result = _mm256_cvtps_epi32(adjusted);
        result = _mm256_min_epi32(result, _mm256_set1_epi32(255));
        result = _mm256_max_epi32(result, _mm256_set1_epi32(0));
        // Speichere das Ergebnis
        _mm256_storeu_si256((__m256i*)&img->data[i], result);
    }
    
    // Verarbeite übrige Pixel
    for (int i = (total_pixels / simd_width) * simd_width; i < total_pixels; i++) {
        float adjusted = img->data[i] * factor;
        img->data[i] = (unsigned char)(adjusted < 0 ? 0 : (adjusted > 255 ? 255 : adjusted));
    }
}

// SIMD-optimierte Gaußsche Weichzeichnung
void gaussian_blur_simd(Image* img, float sigma) {
    int kernel_size = (int)(6 * sigma);
    if (kernel_size % 2 == 0) kernel_size++;
    
    // Erstelle Gauß-Kernel
    float* kernel = (float*)aligned_alloc(32, kernel_size * sizeof(float));
    float sum = 0.0f;
    int half = kernel_size / 2;
    
    for (int i = 0; i < kernel_size; i++) {
        float x = i - half;
        kernel[i] = expf(-(x * x) / (2 * sigma * sigma));
        sum += kernel[i];
    }
    
    // Normalisiere Kernel
    for (int i = 0; i < kernel_size; i++) {
        kernel[i] /= sum;
    }
    
    // Temporärer Speicher für die Verarbeitung
    Image* temp = create_image(img->width, img->height, img->channels);
    
    // Horizontale Faltung mit SIMD
    __m256 kernel_vec;
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                __m256 sum_vec = _mm256_setzero_ps();
                
                for (int k = 0; k < kernel_size; k += 8) {
                    kernel_vec = _mm256_load_ps(&kernel[k]);
                    int px = x - half + k;
                    
                    // Lade 8 benachbarte Pixel
                    __m256i pixels = _mm256_loadu_si256((__m256i*)&img->data[(y * img->width + px) * img->channels + c]);
                    __m256 pixels_float = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_loadu_si128((__m128i*)&img->data[(y * img->width + px) * img->channels + c])));
                    
                    // Multipliziere und addiere
                    sum_vec = _mm256_add_ps(sum_vec, _mm256_mul_ps(pixels_float, kernel_vec));
                }
                
                // Reduziere die Summe
                float sum = 0.0f;
                float temp_sum[8];
                _mm256_store_ps(temp_sum, sum_vec);
                for (int i = 0; i < 8; i++) {
                    sum += temp_sum[i];
                }
                
                temp->data[(y * img->width + x) * img->channels + c] = (unsigned char)(sum + 0.5f);
            }
        }
    }
    
    // Vertikale Faltung
    // [Ähnlicher Code wie horizontale Faltung, mit vertikaler Richtung]
    
    // Kopiere Ergebnis zurück
    memcpy(img->data, temp->data, img->width * img->height * img->channels);
    
    // Aufräumen
    free(kernel);
    free_image(temp);
}

// SIMD-optimierte Kantenerkennung (Sobel-Operator)
void edge_detection_simd(Image* img) {
    if (img->channels != 1) {
        printf("Edge detection requires grayscale image\n");
        return;
    }
    
    Image* temp = create_image(img->width, img->height, 1);
    
    // Sobel-Kernel für x- und y-Richtung
    const __m256i sobel_x = _mm256_set_epi32(-1, 0, 1, -2, 0, 2, -1, 0);
    const __m256i sobel_y = _mm256_set_epi32(-1, -2, -1, 0, 0, 0, 1, 2);
    
    // Verarbeite das Bild in 8er-Blöcken
    for (int y = 1; y < img->height - 1; y++) {
        for (int x = 1; x < img->width - 1; x += 8) {
            __m256i pixels = _mm256_loadu_si256((__m256i*)&img->data[y * img->width + x]);
            
            // Berechne Gradienten in x- und y-Richtung
            __m256i grad_x = _mm256_mullo_epi32(pixels, sobel_x);
            __m256i grad_y = _mm256_mullo_epi32(pixels, sobel_y);
            
            // Berechne Magnitude
            __m256i magnitude = _mm256_add_epi32(
                _mm256_abs_epi32(grad_x),
                _mm256_abs_epi32(grad_y)
            );
            
            // Speichere Ergebnis
            _mm256_storeu_si256((__m256i*)&temp->data[y * img->width + x], magnitude);
        }
    }
    
    // Kopiere Ergebnis zurück
    memcpy(img->data, temp->data, img->width * img->height);
    
    free_image(temp);
}

// Beispiel für die Verwendung der Pipeline
int main() {
    // Erstelle Testbild
    int width = 1920;
    int height = 1080;
    Image* img = create_image(width, height, 3);  // RGB-Bild
    
    // Fülle mit Testdaten
    for (int i = 0; i < width * height * 3; i++) {
        img->data[i] = rand() % 256;
    }
    
    // Wende Pipeline an
    adjust_brightness_simd(img, 1.2f);  // Erhöhe Helligkeit um 20%
    gaussian_blur_simd(img, 1.5f);      // Wende Gaußsche Weichzeichnung an
    
    // Konvertiere zu Graustufen für Kantenerkennung
    Image* gray = create_image(width, height, 1);
    // [Konvertierungscode hier]
    
    edge_detection_simd(gray);
    
    // Aufräumen
    free_image(img);
    free_image(gray);
    
    return 0;
}