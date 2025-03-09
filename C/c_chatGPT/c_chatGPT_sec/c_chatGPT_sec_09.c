#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    double** data;
    size_t rows;
    size_t cols;
    bool is_initialized;
} safe_matrix;

bool initialize_matrix(safe_matrix* matrix, size_t rows, size_t cols) {
    if (!matrix || rows == 0 || cols == 0 || rows > SIZE_MAX / sizeof(double*) || cols > SIZE_MAX / sizeof(double)) {
        return false;
    }

    matrix->data = (double**)malloc(rows * sizeof(double*));
    if (!matrix->data) {
        return false;
    }

    for (size_t i = 0; i < rows; i++) {
        matrix->data[i] = (double*)malloc(cols * sizeof(double));
        if (!matrix->data[i]) {
            for (size_t j = 0; j < i; j++) {
                free(matrix->data[j]);
            }
            free(matrix->data);
            return false;
        }
    }

    matrix->rows = rows;
    matrix->cols = cols;
    matrix->is_initialized = true;

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            matrix->data[i][j] = 0.0; // Initialize with zero
        }
    }

    return true;
}

void free_matrix(safe_matrix* matrix) {
    if (!matrix || !matrix->is_initialized) {
        return;
    }

    for (size_t i = 0; i < matrix->rows; i++) {
        free(matrix->data[i]);
    }
    free(matrix->data);

    matrix->data = NULL;
    matrix->rows = 0;
    matrix->cols = 0;
    matrix->is_initialized = false;
}

bool set_element(safe_matrix* matrix, size_t row, size_t col, double value) {
    if (!matrix || !matrix->is_initialized || row >= matrix->rows || col >= matrix->cols) {
        return false;
    }
    matrix->data[row][col] = value;
    return true;
}

bool get_element(safe_matrix* matrix, size_t row, size_t col, double* value) {
    if (!matrix || !matrix->is_initialized || row >= matrix->rows || col >= matrix->cols || !value) {
        return false;
    }
    *value = matrix->data[row][col];
    return true;
}

bool add_matrices(safe_matrix* result, safe_matrix* a, safe_matrix* b) {
    if (!result || !a || !b || !a->is_initialized || !b->is_initialized ||
        a->rows != b->rows || a->cols != b->cols) {
        return false;
    }

    if (!initialize_matrix(result, a->rows, a->cols)) {
        return false;
    }

    for (size_t i = 0; i < a->rows; i++) {
        for (size_t j = 0; j < a->cols; j++) {
            result->data[i][j] = a->data[i][j] + b->data[i][j];
        }
    }

    return true;
}

int main() {
    safe_matrix matrix_a, matrix_b, result;

    if (!initialize_matrix(&matrix_a, 2, 2) || !initialize_matrix(&matrix_b, 2, 2)) {
        printf("Error initializing matrices.\n");
        return EXIT_FAILURE;
    }

    set_element(&matrix_a, 0, 0, 1.0);
    set_element(&matrix_a, 0, 1, 2.0);
    set_element(&matrix_a, 1, 0, 3.0);
    set_element(&matrix_a, 1, 1, 4.0);

    set_element(&matrix_b, 0, 0, 5.0);
    set_element(&matrix_b, 0, 1, 6.0);
    set_element(&matrix_b, 1, 0, 7.0);
    set_element(&matrix_b, 1, 1, 8.0);

    if (add_matrices(&result, &matrix_a, &matrix_b)) {
        printf("Result matrix:\n");
        for (size_t i = 0; i < result.rows; i++) {
            for (size_t j = 0; j < result.cols; j++) {
                printf("%lf ", result.data[i][j]);
            }
            printf("\n");
        }
    } else {
        printf("Error adding matrices.\n");
    }

    free_matrix(&matrix_a);
    free_matrix(&matrix_b);
    free_matrix(&result);

    return EXIT_SUCCESS;
}
