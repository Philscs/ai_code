#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct Matrix {
    double** data;
    int rows;
    int cols;
};

// Hilfsfunktionen
struct Matrix createMatrix(int rows, int cols) {
    struct Matrix mat;
    mat.rows = rows;
    mat.cols = cols;
    mat.data = (double**)malloc(rows * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        mat.data[i] = (double*)calloc(cols, sizeof(double));
    }
    return mat;
}

void freeMatrix(struct Matrix* mat) {
    for (int i = 0; i < mat->rows; i++) {
        free(mat->data[i]);
    }
    free(mat->data);
}

struct Matrix copyMatrix(struct Matrix src) {
    struct Matrix dest = createMatrix(src.rows, src.cols);
    for (int i = 0; i < src.rows; i++) {
        for (int j = 0; j < src.cols; j++) {
            dest.data[i][j] = src.data[i][j];
        }
    }
    return dest;
}

// Grundoperationen
void swapRows(struct Matrix* mat, int row1, int row2) {
    double* temp = mat->data[row1];
    mat->data[row1] = mat->data[row2];
    mat->data[row2] = temp;
}

struct Matrix matrixMultiply(struct Matrix A, struct Matrix B) {
    if (A.cols != B.rows) {
        printf("Error: Incompatible dimensions for multiplication\n");
        exit(1);
    }
    
    struct Matrix result = createMatrix(A.rows, B.cols);
    for (int i = 0; i < A.rows; i++) {
        for (int j = 0; j < B.cols; j++) {
            for (int k = 0; k < A.cols; k++) {
                result.data[i][j] += A.data[i][k] * B.data[k][j];
            }
        }
    }
    return result;
}

// Gauss-Elimination
struct Matrix gaussElimination(struct Matrix A, struct Matrix b) {
    if (A.rows != A.cols || A.rows != b.rows || b.cols != 1) {
        printf("Error: Invalid dimensions for Gauss elimination\n");
        exit(1);
    }

    int n = A.rows;
    struct Matrix augmented = createMatrix(n, n + 1);
    
    // Erstelle erweiterte Matrix [A|b]
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            augmented.data[i][j] = A.data[i][j];
        }
        augmented.data[i][n] = b.data[i][0];
    }

    // Vorwärtselimination
    for (int k = 0; k < n - 1; k++) {
        // Pivotierung
        int maxRow = k;
        double maxVal = fabs(augmented.data[k][k]);
        for (int i = k + 1; i < n; i++) {
            if (fabs(augmented.data[i][k]) > maxVal) {
                maxVal = fabs(augmented.data[i][k]);
                maxRow = i;
            }
        }
        if (maxRow != k) {
            swapRows(&augmented, k, maxRow);
        }

        for (int i = k + 1; i < n; i++) {
            double factor = augmented.data[i][k] / augmented.data[k][k];
            for (int j = k; j <= n; j++) {
                augmented.data[i][j] -= factor * augmented.data[k][j];
            }
        }
    }

    // Rückwärtssubstitution
    struct Matrix x = createMatrix(n, 1);
    for (int i = n - 1; i >= 0; i--) {
        x.data[i][0] = augmented.data[i][n];
        for (int j = i + 1; j < n; j++) {
            x.data[i][0] -= augmented.data[i][j] * x.data[j][0];
        }
        x.data[i][0] /= augmented.data[i][i];
    }

    freeMatrix(&augmented);
    return x;
}

// LU-Zerlegung
void luDecomposition(struct Matrix A, struct Matrix* L, struct Matrix* U, struct Matrix* P) {
    if (A.rows != A.cols) {
        printf("Error: Matrix must be square for LU decomposition\n");
        exit(1);
    }

    int n = A.rows;
    *L = createMatrix(n, n);
    *U = copyMatrix(A);
    *P = createMatrix(n, n);

    // Initialisiere Permutationsmatrix als Einheitsmatrix
    for (int i = 0; i < n; i++) {
        P->data[i][i] = 1.0;
    }

    for (int k = 0; k < n - 1; k++) {
        // Pivotierung
        int maxRow = k;
        double maxVal = fabs(U->data[k][k]);
        for (int i = k + 1; i < n; i++) {
            if (fabs(U->data[i][k]) > maxVal) {
                maxVal = fabs(U->data[i][k]);
                maxRow = i;
            }
        }
        if (maxRow != k) {
            swapRows(U, k, maxRow);
            swapRows(P, k, maxRow);
            swapRows(L, k, maxRow);
        }

        for (int i = k + 1; i < n; i++) {
            L->data[i][k] = U->data[i][k] / U->data[k][k];
            for (int j = k; j < n; j++) {
                U->data[i][j] -= L->data[i][k] * U->data[k][j];
            }
        }
    }

    // Setze Diagonalelemente von L auf 1
    for (int i = 0; i < n; i++) {
        L->data[i][i] = 1.0;
    }
}

// Power-Iteration für dominanten Eigenwert
void powerIteration(struct Matrix A, double* eigenvalue, struct Matrix* eigenvector, int maxIter, double tol) {
    if (A.rows != A.cols) {
        printf("Error: Matrix must be square for eigenvalue computation\n");
        exit(1);
    }

    int n = A.rows;
    *eigenvector = createMatrix(n, 1);
    
    // Initialisiere Eigenvektor
    for (int i = 0; i < n; i++) {
        eigenvector->data[i][0] = 1.0 / sqrt(n);
    }

    double lambda_old = 0.0;
    for (int iter = 0; iter < maxIter; iter++) {
        // Matrixmultiplikation: y = A * x
        struct Matrix y = matrixMultiply(A, *eigenvector);
        
        // Finde Komponente mit größtem Betrag für Normierung
        double max_comp = 0.0;
        for (int i = 0; i < n; i++) {
            if (fabs(y.data[i][0]) > fabs(max_comp)) {
                max_comp = y.data[i][0];
            }
        }
        
        // Normiere Vektor
        for (int i = 0; i < n; i++) {
            eigenvector->data[i][0] = y.data[i][0] / max_comp;
        }
        
        // Berechne Rayleigh-Quotienten für Eigenwert
        struct Matrix temp1 = matrixMultiply(A, *eigenvector);
        double numerator = 0.0, denominator = 0.0;
        for (int i = 0; i < n; i++) {
            numerator += temp1.data[i][0] * eigenvector->data[i][0];
            denominator += eigenvector->data[i][0] * eigenvector->data[i][0];
        }
        *eigenvalue = numerator / denominator;
        
        // Überprüfe Konvergenz
        if (fabs(*eigenvalue - lambda_old) < tol) {
            break;
        }
        lambda_old = *eigenvalue;
        
        freeMatrix(&y);
        freeMatrix(&temp1);
    }
}

// Beispielnutzung
void printMatrix(struct Matrix mat) {
    for (int i = 0; i < mat.rows; i++) {
        for (int j = 0; j < mat.cols; j++) {
            printf("%f\t", mat.data[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int main() {
    // Beispiel für Gauss-Elimination
    struct Matrix A = createMatrix(3, 3);
    A.data[0][0] = 2; A.data[0][1] = 1; A.data[0][2] = 1;
    A.data[1][0] = 1; A.data[1][1] = 3; A.data[1][2] = 2;
    A.data[2][0] = 1; A.data[2][1] = 0; A.data[2][2] = 0;
    
    struct Matrix b = createMatrix(3, 1);
    b.data[0][0] = 4;
    b.data[1][0] = 5;
    b.data[2][0] = 6;
    
    printf("Lösung des Gleichungssystems Ax = b:\n");
    struct Matrix x = gaussElimination(A, b);
    printMatrix(x);
    
    // Beispiel für LU-Zerlegung
    struct Matrix L, U, P;
    luDecomposition(A, &L, &U, &P);
    printf("LU-Zerlegung:\nL:\n");
    printMatrix(L);
    printf("U:\n");
    printMatrix(U);
    printf("P:\n");
    printMatrix(P);
    
    // Beispiel für Eigenwertberechnung
    double eigenvalue;
    struct Matrix eigenvector;
    powerIteration(A, &eigenvalue, &eigenvector, 1000, 1e-10);
    printf("Dominanter Eigenwert: %f\n", eigenvalue);
    printf("Zugehöriger Eigenvektor:\n");
    printMatrix(eigenvector);
    
    // Aufräumen
    freeMatrix(&A);
    freeMatrix(&b);
    freeMatrix(&x);
    freeMatrix(&L);
    freeMatrix(&U);
    freeMatrix(&P);
    freeMatrix(&eigenvector);
    
    return 0;
}