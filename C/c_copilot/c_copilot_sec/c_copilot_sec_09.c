#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

typedef struct {
    double** data;
    size_t rows;
    size_t cols;
    bool is_initialized;
} safe_matrix;

safe_matrix* create_safe_matrix(size_t rows, size_t cols) {
    safe_matrix* matrix = malloc(sizeof(safe_matrix));
    if (matrix == NULL) {
        fprintf(stderr, "Failed to allocate memory for matrix\n");
        return NULL;
    }

    matrix->data = malloc(rows * sizeof(double*));
    if (matrix->data == NULL) {
        fprintf(stderr, "Failed to allocate memory for matrix data\n");
        free(matrix);
        return NULL;
    }

    for (size_t i = 0; i < rows; i++) {
        matrix->data[i] = malloc(cols * sizeof(double));
        if (matrix->data[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for matrix row\n");
            for (size_t j = 0; j < i; j++) {
                free(matrix->data[j]);
            }
            free(matrix->data);
            free(matrix);
            return NULL;
        }
    }

    matrix->rows = rows;
    matrix->cols = cols;
    matrix->is_initialized = true;

    return matrix;
}

void destroy_safe_matrix(safe_matrix* matrix) {
    if (matrix == NULL) {
        return;
    }

    for (size_t i = 0; i < matrix->rows; i++) {
        free(matrix->data[i]);
    }
    free(matrix->data);
    free(matrix);
}

double get_element(safe_matrix* matrix, size_t row, size_t col) {
    if (matrix == NULL || !matrix->is_initialized) {
        fprintf(stderr, "Matrix is not initialized\n");
        return 0.0;
    }

    if (row >= matrix->rows || col >= matrix->cols) {
        fprintf(stderr, "Invalid matrix indices\n");
        return 0.0;
    }

    return matrix->data[row][col];
}

void set_element(safe_matrix* matrix, size_t row, size_t col, double value) {
    if (matrix == NULL || !matrix->is_initialized) {
        fprintf(stderr, "Matrix is not initialized\n");
        return;
    }

    if (row >= matrix->rows || col >= matrix->cols) {
        fprintf(stderr, "Invalid matrix indices\n");
        return;
    }

    matrix->data[row][col] = value;
}

size_t get_num_rows(safe_matrix* matrix) {
    if (matrix == NULL || !matrix->is_initialized) {
        fprintf(stderr, "Matrix is not initialized\n");
        return 0;
    }

    return matrix->rows;
}

size_t get_num_cols(safe_matrix* matrix) {
    if (matrix == NULL || !matrix->is_initialized) {
        fprintf(stderr, "Matrix is not initialized\n");
        return 0;
    }

    return matrix->cols;
}

bool is_initialized(safe_matrix* matrix) {
    if (matrix == NULL) {
        return false;
    }

    return matrix->is_initialized;
}

bool is_valid_index(safe_matrix* matrix, size_t row, size_t col) {
    if (matrix == NULL || !matrix->is_initialized) {
        return false;
    }

    return row < matrix->rows && col < matrix->cols;
}
