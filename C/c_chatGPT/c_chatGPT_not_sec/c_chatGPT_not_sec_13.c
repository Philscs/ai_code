#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Matrix-Struktur
typedef struct {
    double** data;
    int rows;
    int cols;
} Matrix;

// Speicher für eine Matrix reservieren
Matrix createMatrix(int rows, int cols) {
    Matrix mat;
    mat.rows = rows;
    mat.cols = cols;
    mat.data = (double**)malloc(rows * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        mat.data[i] = (double*)calloc(cols, sizeof(double));
    }
    return mat;
}

// Speicher freigeben
void freeMatrix(Matrix mat) {
    for (int i = 0; i < mat.rows; i++) {
        free(mat.data[i]);
    }
    free(mat.data);
}

// Matrix drucken
void printMatrix(Matrix mat) {
    for (int i = 0; i < mat.rows; i++) {
        for (int j = 0; j < mat.cols; j++) {
            printf("%.2f ", mat.data[i][j]);
        }
        printf("\n");
    }
}

// Gauss-Elimination
void gaussElimination(Matrix mat) {
    for (int i = 0; i < mat.rows; i++) {
        // Pivotisierung
        for (int k = i + 1; k < mat.rows; k++) {
            if (fabs(mat.data[k][i]) > fabs(mat.data[i][i])) {
                double* temp = mat.data[i];
                mat.data[i] = mat.data[k];
                mat.data[k] = temp;
            }
        }

        // Eliminierung
        for (int k = i + 1; k < mat.rows; k++) {
            double factor = mat.data[k][i] / mat.data[i][i];
            for (int j = i; j < mat.cols; j++) {
                mat.data[k][j] -= factor * mat.data[i][j];
            }
        }
    }
}

// LU-Dekomposition
void luDecomposition(Matrix mat, Matrix* L, Matrix* U) {
    *L = createMatrix(mat.rows, mat.cols);
    *U = createMatrix(mat.rows, mat.cols);

    for (int i = 0; i < mat.rows; i++) {
        for (int j = 0; j < mat.cols; j++) {
            if (j < i)
                L->data[j][i] = 0;
            else {
                L->data[j][i] = mat.data[j][i];
                for (int k = 0; k < i; k++) {
                    L->data[j][i] -= L->data[j][k] * U->data[k][i];
                }
            }

            if (j < i)
                U->data[i][j] = 0;
            else if (j == i)
                U->data[i][j] = 1;
            else {
                U->data[i][j] = mat.data[i][j] / L->data[i][i];
                for (int k = 0; k < i; k++) {
                    U->data[i][j] -= ((L->data[i][k] * U->data[k][j]) / L->data[i][i]);
                }
            }
        }
    }
}

// Eigenwert-Berechnung mit der Potenzmethode
double powerMethod(Matrix mat, double* eigenVector, int maxIterations, double tolerance) {
    double eigenValue = 0.0;
    double* tempVector = (double*)malloc(mat.rows * sizeof(double));

    // Startvektor initialisieren
    for (int i = 0; i < mat.rows; i++) {
        eigenVector[i] = 1.0;
    }

    for (int iter = 0; iter < maxIterations; iter++) {
        // Matrix-Vektor-Produkt
        for (int i = 0; i < mat.rows; i++) {
            tempVector[i] = 0.0;
            for (int j = 0; j < mat.cols; j++) {
                tempVector[i] += mat.data[i][j] * eigenVector[j];
            }
        }

        // Norm des Vektors berechnen
        double norm = 0.0;
        for (int i = 0; i < mat.rows; i++) {
            norm += tempVector[i] * tempVector[i];
        }
        norm = sqrt(norm);

        // Vektor normalisieren
        for (int i = 0; i < mat.rows; i++) {
            tempVector[i] /= norm;
        }

        // Eigenwert berechnen
        double newEigenValue = 0.0;
        for (int i = 0; i < mat.rows; i++) {
            newEigenValue += tempVector[i] * eigenVector[i];
        }

        // Konvergenz überprüfen
        if (fabs(newEigenValue - eigenValue) < tolerance) {
            eigenValue = newEigenValue;
            break;
        }

        eigenValue = newEigenValue;

        // Vektor aktualisieren
        for (int i = 0; i < mat.rows; i++) {
            eigenVector[i] = tempVector[i];
        }
    }

    free(tempVector);
    return eigenValue;
}

int main() {
    Matrix mat = createMatrix(3, 3);
    mat.data[0][0] = 2; mat.data[0][1] = -1; mat.data[0][2] = 0;
    mat.data[1][0] = -1; mat.data[1][1] = 2; mat.data[1][2] = -1;
    mat.data[2][0] = 0; mat.data[2][1] = -1; mat.data[2][2] = 2;

    printf("Original Matrix:\n");
    printMatrix(mat);

    // Gauss-Elimination
    gaussElimination(mat);
    printf("\nAfter Gauss Elimination:\n");
    printMatrix(mat);

    // LU-Decomposition
    Matrix L, U;
    luDecomposition(mat, &L, &U);
    printf("\nL Matrix:\n");
    printMatrix(L);
    printf("\nU Matrix:\n");
    printMatrix(U);

    // Eigenwertberechnung
    double eigenVector[3];
    double eigenValue = powerMethod(mat, eigenVector, 1000, 1e-6);
    printf("\nEigenwert: %.6f\n", eigenValue);

    printf("Eigenvektor: [ ");
    for (int i = 0; i < mat.rows; i++) {
        printf("%.6f ", eigenVector[i]);
    }
    printf("]\n");

    // Speicher freigeben
    freeMatrix(mat);
    freeMatrix(L);
    freeMatrix(U);

    return 0;
}
