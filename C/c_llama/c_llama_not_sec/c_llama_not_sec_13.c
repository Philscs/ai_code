#include <stdio.h>
#include <stdlib.h>

// Struktur für Matrizen
struct Matrix {
    double** data;
    int rows;
    int cols;
};

// Funktion zur Erstellung einer neuen Matrix
struct Matrix* createMatrix(int rows, int cols) {
    struct Matrix* matrix = (struct Matrix*)malloc(sizeof(struct Matrix));
    if (!matrix) return NULL;

    matrix->rows = rows;
    matrix->cols = cols;
    matrix->data = (double**)malloc(rows * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        matrix->data[i] = (double*)malloc(cols * sizeof(double));
    }

    return matrix;
}

// Funktion zum Ausgabe der Matrix
void printMatrix(struct Matrix* matrix) {
    for (int i = 0; i < matrix->rows; i++) {
        for (int j = 0; j < matrix->cols; j++) {
            printf("%.2f ", matrix->data[i][j]);
        }
        printf("\n");
    }
}

// Funktion zur Auswertung einer Matrix
void evaluateMatrix(struct Matrix* matrix) {
    for (int i = 0; i < matrix->rows; i++) {
        for (int j = 0; j < matrix->cols; j++) {
            printf("Element %d, %d: %.2f\n", i + 1, j + 1, matrix->data[i][j]);
        }
    }
}

// Funktion für Gauss-Elimination
void gaussElimination(struct Matrix* A) {
    for (int i = 0; i < A->rows - 1; i++) {
        // Suche nach dem größten Abstand von der Diagonale
        int maxRow = i;
        double maxDistance = abs(A->data[i][i]);
        for (int j = i + 1; j < A->rows; j++) {
            if (abs(A->data[j][i]) > maxDistance) {
                maxRow = j;
                maxDistance = abs(A->data[j][i]);
            }
        }

        // Swapper für die Diagonale
        double temp = A->data[i][i];
        A->data[i][i] = A->data[maxRow][i];
        A->data[maxRow][i] = temp;

        // Elimination
        for (int j = i + 1; j < A->rows; j++) {
            double factor = A->data[j][i] / A->data[i][i];
            for (int k = i; k < A->cols; k++) {
                A->data[j][k] -= factor * A->data[i][k];
            }
        }
    }

    printf("Gauss-Elimination resultiert in:\n");
    printMatrix(A);
}

// Funktion für LU-Dekomposition
void luDecomposition(struct Matrix* A) {
    struct Matrix* L = createMatrix(A->rows, A->cols);

    for (int i = 0; i < A->rows; i++) {
        // Erstellung der obigen Spalte
        for (int j = 0; j <= i; j++) {
            L->data[i][j] = (i == j) ? 1.0 : 0.0;
        }

        // Elimination
        for (int k = i + 1; k < A->rows; k++) {
            double factor = A->data[k][i];
            for (int l = i; l < A->cols; l++) {
                L->data[k][l] += factor * L->data[i][l];
            }
        }

        printf("LU-Dekomposition resultiert in:\n");
        printMatrix(L);
    }

    printf("\nL =\n");
    printMatrix(L);

    struct Matrix* U = createMatrix(A->rows, A->cols);
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < A->cols; j++) {
            U->data[i][j] = (i == j) ? A->data[i][j] : 0.0;
        }
    }

    printf("\nU =\n");
    printMatrix(U);
}

// Funktion zur Berechnung von Eigenwerten
void eigenValue(struct Matrix* A) {
    // Wichtige Hinweis: Diese Berechnung ist für eine breitere Matrizenbasis nicht geeignet.
    // Eine Lösung hierfür wäre ein Bibliotheksaufruf wie LAPACK oder MKL.

    double eigenvalue = 0;
    for (int i = 0; i < A->rows; i++) {
        eigenvalue += A->data[i][i];
    }

    printf("Eigenwert: %.2f\n", eigenvalue);
}

// Funktion zur Freigabe der Matrix
void freeMatrix(struct Matrix* matrix) {
    if (matrix == NULL) return;

    for (int i = 0; i < matrix->rows; i++) {
        free(matrix->data[i]);
    }

    free(matrix->data);

    free(matrix);
}

// Beispiel für eine Matrix
struct Matrix A = { createMatrix(3, 3), 3, 3 };
A.data[0][0] = 2.0;
A.data[1][1] = -4.0;
A.data[2][2] = 5.0;

int main() {
    gaussElimination(&A);
    luDecomposition(&A);
    eigenValue(&A);

    freeMatrix(&A);

    return 0;
}
