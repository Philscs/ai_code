#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE2

#define WIDTH 1024
#define HEIGHT 768

void processImageSSE(const unsigned char* input, unsigned char* output, int width, int height) {
    int numPixels = width * height;

    // Process 16 pixels at a time (4 channels: RGBA * 4 = 16 bytes per operation)
    for (int i = 0; i < numPixels * 4; i += 16) {
        // Load 16 bytes from the input image
        __m128i pixels = _mm_loadu_si128((__m128i*)(input + i));

        // Example operation: Increase brightness by a fixed value (here: 50)
        __m128i brightness = _mm_set1_epi8(50); // Set all 16 values to 50
        __m128i result = _mm_adds_epu8(pixels, brightness); // Saturated addition

        // Store the result in the output image
        _mm_storeu_si128((__m128i*)(output + i), result);
    }
}

int main() {
    // Memory for the input and output images
    unsigned char* inputImage = (unsigned char*)aligned_alloc(16, WIDTH * HEIGHT * 4);
    unsigned char* outputImage = (unsigned char*)aligned_alloc(16, WIDTH * HEIGHT * 4);

    if (!inputImage || !outputImage) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }

    // Example: Initialize the input image (random pixel values)
    for (int i = 0; i < WIDTH * HEIGHT * 4; i++) {
        inputImage[i] = rand() % 256;
    }

    // Process the image
    processImageSSE(inputImage, outputImage, WIDTH, HEIGHT);

    // Save or further process the result (e.g., save as a file)

    // Free memory
    free(inputImage);
    free(outputImage);

    return 0;
}
