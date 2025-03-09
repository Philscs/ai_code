#include <stdio.h>
#include <stdlib.h>

typedef struct Student {
    int matrikelnummer;
    char name[50];
    float notendurchschnitt;
    int color; // 0 for black, 1 for red
    struct Student* parent;
    struct Student* left;
    struct Student* right;
} Student;

Student* createStudent(int matrikelnummer, char* name, float notendurchschnitt) {
    Student* student = (Student*)malloc(sizeof(Student));
    student->matrikelnummer = matrikelnummer;
    strcpy(student->name, name);
    student->notendurchschnitt = notendurchschnitt;
    student->color = 1; // new nodes are always red
    student->parent = NULL;
    student->left = NULL;
    student->right = NULL;
    return student;
}

void insert(Student** root, Student* student) {
    // TODO: Implement the insert operation
}

void delete(Student** root, int matrikelnummer) {
    // TODO: Implement the delete operation
}

void printInorder(Student* student) {
    if (student != NULL) {
        printInorder(student->left);
        printf("Matrikelnummer: %d, Name: %s, Notendurchschnitt: %.2f\n", student->matrikelnummer, student->name, student->notendurchschnitt);
        printInorder(student->right);
    }
}

int main() {
    Student* root = NULL;

    // Example usage
    insert(&root, createStudent(12345, "John Doe", 3.8));
    insert(&root, createStudent(54321, "Jane Smith", 4.2));
    insert(&root, createStudent(98765, "Alice Johnson", 3.5));

    printf("Inorder traversal:\n");
    printInorder(root);

    return 0;
}
