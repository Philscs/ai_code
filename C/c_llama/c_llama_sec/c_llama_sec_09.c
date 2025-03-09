#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// Typdef für die Matrix-Struktur
typedef struct {
    double** data;
    size_t rows;
    size_t cols;
    bool is_initialized;
} safe_matrix;

// Funktion, um eine neue Matrix zu erstellen
safe_matrix* matrix_new(size_t rows, size_t cols) {
    assert(rows > 0 && cols > 0);
    
    // Allokieren der Zeilen
    safe_matrix* mat = malloc(sizeof(safe_matrix));
    mat->data = malloc(rows * sizeof(double*));
    for (size_t i = 0; i < rows; i++) {
        mat->data[i] = malloc(cols * sizeof(double));
    }
    
    // Einstellung der Größe
    mat->rows = rows;
    mat->cols = cols;
    mat->is_initialized = false;
    
    return mat;
}

// Funktion, um eine Matrix zu befreien
void matrix_free(safe_matrix* mat) {
    if (mat != NULL) {
        for (size_t i = 0; i < mat->rows; i++) {
            free(mat->data[i]);
        }
        free(mat->data);
        free(mat);
    }
}

// Funktion, um einen Wert in einer Matrix zu setzen
void matrix_set(safe_matrix* mat, size_t row, size_t col, double value) {
    assert(row < mat->rows && col < mat->cols);
    
    // Überprüfung auf Integer-Overflow bei der Berechnung von mat->data[row][col]
    if (row >= mat->rows || col >= mat->cols) {
        printf("Boundary-Check für Matrix-Access gefehlt!\n");
        return;
    }
    
    mat->data[row][col] = value;
}

// Funktion, um einen Wert aus einer Matrix abzulesen
double matrix_get(safe_matrix* mat, size_t row, size_t col) {
    assert(row < mat->rows && col < mat->cols);
    
    // Überprüfung auf Integer-Overflow bei der Berechnung von mat->data[row][col]
    if (row >= mat->rows || col >= mat->cols) {
        printf("Boundary-Check für Matrix-Access gefehlt!\n");
        return 0.0;
    }
    
    return mat->data[row][col];
}

// Funktion, um eine Matrix zu multiplizieren
safe_matrix* matrix_multiply(safe_matrix* a, safe_matrix* b) {
    assert(a != NULL && b != NULL);
    
    // Überprüfung auf Kompatibilität zwischen den Matrizen
    if (a->cols != b->rows) {
        printf("Inkompatible Matrix-Größen für Multiplikation!\n");
        return NULL;
    }
    
    // Allokieren der Ergebnismatrix
    safe_matrix* result = matrix_new(a->rows, b->cols);
    
    for (size_t i = 0; i < a->rows; i++) {
        for (size_t j = 0; j < b->cols; j++) {
            // Berechnung des Ergebnisses mithilfe von elementaren Matrixmultiplikation
            double sum = 0.0;
            for (size_t k = 0; k < a->cols; k++) {
                sum += a->data[i][k] * b->data[k][j];
            }
            matrix_set(result, i, j, sum);
        }
    }
    
    return result;
}

int main() {
    // Erstellen einer neuen Matrix
    safe_matrix* mat = matrix_new(2, 3);
    assert(mat != NULL);
    
    // Setze Werte in die Matrix
    for (size_t i = 0; i < 2; i++) {
        for (size_t j = 0; j < 3; j++) {
            printf("Wert an Position (%d, %d) = ", i, j);
            matrix_set(mat, i, j, i + j);
        }
    }
    
    // Lese Werte aus der Matrix
    for (size_t i = 0; i < 2; i++) {
        for (size_t j = 0; j < 3; j++) {
            printf("Wert an Position (%d, %d) = %f\n", i, j, matrix_get(mat, i, j));
        }
    }
    
    // Freie der Matrix
    matrix_free(mat);
    
    return 0;
}