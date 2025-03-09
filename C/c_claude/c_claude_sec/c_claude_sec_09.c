#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

typedef struct {
    double** data;
    size_t rows;
    size_t cols;
    bool is_initialized;
} safe_matrix;

// Sicherheitsprüfungen für Dimensionen
bool is_valid_dimensions(size_t rows, size_t cols) {
    // Prüfe auf Null-Dimensionen
    if (rows == 0 || cols == 0) return false;
    
    // Prüfe auf Integer Overflow bei der Multiplikation
    if (rows > SIZE_MAX / cols) return false;
    
    // Prüfe ob die Gesamtgröße im erlaubten Bereich liegt
    size_t total_size;
    if (__builtin_mul_overflow(rows, cols, &total_size)) return false;
    if (__builtin_mul_overflow(total_size, sizeof(double), &total_size)) return false;
    
    return true;
}

// Initialisierung der Matrix
safe_matrix* create_matrix(size_t rows, size_t cols) {
    if (!is_valid_dimensions(rows, cols)) {
        errno = EOVERFLOW;
        return NULL;
    }

    safe_matrix* matrix = calloc(1, sizeof(safe_matrix));
    if (!matrix) {
        errno = ENOMEM;
        return NULL;
    }

    // Allokiere Zeilen-Array
    matrix->data = calloc(rows, sizeof(double*));
    if (!matrix->data) {
        free(matrix);
        errno = ENOMEM;
        return NULL;
    }

    // Allokiere einzelne Zeilen
    for (size_t i = 0; i < rows; i++) {
        matrix->data[i] = calloc(cols, sizeof(double));
        if (!matrix->data[i]) {
            // Cleanup bei Fehler
            for (size_t j = 0; j < i; j++) {
                free(matrix->data[j]);
            }
            free(matrix->data);
            free(matrix);
            errno = ENOMEM;
            return NULL;
        }
    }

    matrix->rows = rows;
    matrix->cols = cols;
    matrix->is_initialized = true;
    return matrix;
}

// Sicheres Setzen eines Matrix-Elements
bool set_element(safe_matrix* matrix, size_t row, size_t col, double value) {
    if (!matrix || !matrix->is_initialized) {
        errno = EINVAL;
        return false;
    }

    if (row >= matrix->rows || col >= matrix->cols) {
        errno = ERANGE;
        return false;
    }

    matrix->data[row][col] = value;
    return true;
}

// Sicheres Lesen eines Matrix-Elements
bool get_element(const safe_matrix* matrix, size_t row, size_t col, double* result) {
    if (!matrix || !matrix->is_initialized || !result) {
        errno = EINVAL;
        return false;
    }

    if (row >= matrix->rows || col >= matrix->cols) {
        errno = ERANGE;
        return false;
    }

    *result = matrix->data[row][col];
    return true;
}

// Matrix-Addition mit Overflow-Prüfung
safe_matrix* add_matrices(const safe_matrix* a, const safe_matrix* b) {
    if (!a || !b || !a->is_initialized || !b->is_initialized) {
        errno = EINVAL;
        return NULL;
    }

    if (a->rows != b->rows || a->cols != b->cols) {
        errno = EINVAL;
        return NULL;
    }

    safe_matrix* result = create_matrix(a->rows, a->cols);
    if (!result) return NULL;

    for (size_t i = 0; i < a->rows; i++) {
        for (size_t j = 0; j < a->cols; j++) {
            result->data[i][j] = a->data[i][j] + b->data[i][j];
        }
    }

    return result;
}

// Sichere Freigabe des Speichers
void free_matrix(safe_matrix* matrix) {
    if (!matrix || !matrix->is_initialized) return;

    if (matrix->data) {
        for (size_t i = 0; i < matrix->rows; i++) {
            free(matrix->data[i]);
        }
        free(matrix->data);
    }

    matrix->is_initialized = false;
    free(matrix);
}

// Matrix-Multiplikation mit Sicherheitsprüfungen
safe_matrix* multiply_matrices(const safe_matrix* a, const safe_matrix* b) {
    if (!a || !b || !a->is_initialized || !b->is_initialized) {
        errno = EINVAL;
        return NULL;
    }

    if (a->cols != b->rows) {
        errno = EINVAL;
        return NULL;
    }

    safe_matrix* result = create_matrix(a->rows, b->cols);
    if (!result) return NULL;

    for (size_t i = 0; i < a->rows; i++) {
        for (size_t j = 0; j < b->cols; j++) {
            double sum = 0.0;
            for (size_t k = 0; k < a->cols; k++) {
                sum += a->data[i][k] * b->data[k][j];
            }
            result->data[i][j] = sum;
        }
    }

    return result;
}

// Transponieren einer Matrix
safe_matrix* transpose_matrix(const safe_matrix* matrix) {
    if (!matrix || !matrix->is_initialized) {
        errno = EINVAL;
        return NULL;
    }

    safe_matrix* result = create_matrix(matrix->cols, matrix->rows);
    if (!result) return NULL;

    for (size_t i = 0; i < matrix->rows; i++) {
        for (size_t j = 0; j < matrix->cols; j++) {
            result->data[j][i] = matrix->data[i][j];
        }
    }

    return result;
}