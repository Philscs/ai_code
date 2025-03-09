#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define BLOCK_SIZE 8
#define PI 3.14159265358979323846

// DCT Transformation
void dct(double block[BLOCK_SIZE][BLOCK_SIZE]) {
    double temp[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double alpha_u, alpha_v;

    for (int u = 0; u < BLOCK_SIZE; u++) {
        for (int v = 0; v < BLOCK_SIZE; v++) {
            alpha_u = (u == 0) ? 1.0 / sqrt(2.0) : 1.0;
            alpha_v = (v == 0) ? 1.0 / sqrt(2.0) : 1.0;
            double sum = 0.0;
            for (int x = 0; x < BLOCK_SIZE; x++) {
                for (int y = 0; y < BLOCK_SIZE; y++) {
                    sum += block[x][y] * 
                           cos((2 * x + 1) * u * PI / (2 * BLOCK_SIZE)) * 
                           cos((2 * y + 1) * v * PI / (2 * BLOCK_SIZE));
                }
            }
            temp[u][v] = 0.25 * alpha_u * alpha_v * sum;
        }
    }

    // Copy back the result
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] = temp[i][j];
        }
    }
}

// Inverse DCT Transformation
void idct(double block[BLOCK_SIZE][BLOCK_SIZE]) {
    double temp[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double alpha_u, alpha_v;

    for (int x = 0; x < BLOCK_SIZE; x++) {
        for (int y = 0; y < BLOCK_SIZE; y++) {
            double sum = 0.0;
            for (int u = 0; u < BLOCK_SIZE; u++) {
                for (int v = 0; v < BLOCK_SIZE; v++) {
                    alpha_u = (u == 0) ? 1.0 / sqrt(2.0) : 1.0;
                    alpha_v = (v == 0) ? 1.0 / sqrt(2.0) : 1.0;
                    sum += alpha_u * alpha_v * block[u][v] * 
                           cos((2 * x + 1) * u * PI / (2 * BLOCK_SIZE)) * 
                           cos((2 * y + 1) * v * PI / (2 * BLOCK_SIZE));
                }
            }
            temp[x][y] = 0.25 * sum;
        }
    }

    // Copy back the result
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] = temp[i][j];
        }
    }
}

// Simplified Quantization Table
int quant_table[BLOCK_SIZE][BLOCK_SIZE] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};

void quantize(double block[BLOCK_SIZE][BLOCK_SIZE]) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] = round(block[i][j] / quant_table[i][j]);
        }
    }
}

void dequantize(double block[BLOCK_SIZE][BLOCK_SIZE]) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] = block[i][j] * quant_table[i][j];
        }
    }
}

// Example main function
int main() {
    double block[BLOCK_SIZE][BLOCK_SIZE] = {
        {52, 55, 61, 66, 70, 61, 64, 73},
        {63, 59, 66, 90, 109, 85, 69, 72},
        {62, 59, 68, 113, 144, 104, 66, 73},
        {63, 58, 71, 122, 154, 106, 70, 69},
        {67, 61, 68, 104, 126, 88, 68, 70},
        {79, 65, 60, 70, 77, 68, 58, 75},
        {85, 71, 64, 59, 55, 61, 65, 83},
        {87, 79, 69, 68, 65, 76, 78, 94}
    };

    printf("Original Block:\n");
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            printf("%6.2f ", block[i][j]);
        }
        printf("\n");
    }

    dct(block);
    printf("\nAfter DCT:\n");
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            printf("%6.2f ", block[i][j]);
        }
        printf("\n");
    }

    quantize(block);
    printf("\nAfter Quantization:\n");
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            printf("%6.2f ", block[i][j]);
        }
        printf("\n");
    }

    dequantize(block);
    idct(block);
    printf("\nAfter Dequantization and IDCT:\n");
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            printf("%6.2f ", block[i][j]);
        }
        printf("\n");
    }

    return 0;
}
