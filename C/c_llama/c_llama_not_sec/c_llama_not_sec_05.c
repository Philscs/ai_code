#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Struktur für eine Datei
typedef struct {
    char Name[20];
    int Size;
} File;

// Struktur für ein Verzeichnis
typedef struct {
    int Parent;
    int NumChildren;
    File** Children;
} Directory;

// Struktur für den File-System
typedef struct {
    Directory RootDir;
    int CurrentPos;
    FILE* Journal;
} FileSystem;

#define FAT_SIZE 1024
#define MAX_NAME_LENGTH 20
#define MAX_FILE_SIZE 1000000

void createFat(FILE* journal) {
    // Erstelle die FAT
    File fat[FAT_SIZE];
    for (int i = 0; i < FAT_SIZE; i++) {
        fat[i].Size = 0;
    }

    // Schreibe die FAT in das Journal
    fwrite(&fat, sizeof(File), 1, journal);
}

void writeFile(const char* filename, int size) {
    // Suche nach einem freien Platz in der FAT
    for (int i = 0; i < FAT_SIZE; i++) {
        if (fat[i].Size == 0) {
            fat[i].Size = size;
            strcpy(fat[i].Name, filename);
            return;
        }
    }

    // Fehlendes Speichermittel
    printf("Fehlendes Speichermittel\n");
}

void writeFile(const char* filename, int size) {
    // Suche nach dem File in der FAT
    for (int i = 0; i < FAT_SIZE; i++) {
        if (strcmp(fat[i].Name, filename) == 0 && fat[i].Size > 0) {
            // Löschen des Files
            fat[i].Size = 0;
            strcpy(fat[i].Name, "");
            return;
        }
    }

    // File nicht gefunden
    printf("File nicht gefunden\n");
}

void createJournal() {
    journal = fopen("journal.dat", "wb+");
}

// Schreiben der FAT in das Journal
void writeFatToJournal() {
    fwrite(&fat, sizeof(File), 1, journal);
}

void closeJournal() {
    fclose(journal);
}

int main() {
    createJournal();

    // Erstelle einen neuen Directory
    Directory dir;
    dir.Parent = -1;
    dir.NumChildren = 0;
    dir.Children = (File**)malloc(10 * sizeof(File*));
    for (int i = 0; i < 10; i++) {
        dir.Children[i] = NULL;
    }

    // Schreibe die FAT in das Journal
    createFat(journal);

    // Erstelle eine neue Datei
    writeFile("date.txt", 100);

    // Öffne den File-System
    FileSystem fs;
    fs.RootDir = dir;
    fs.CurrentPos = 0;
    fs.Journal = journal;

    // Öffne das Journal und schreibe die FAT
    closeJournal();
    fs.writeFatToJournal();

    return 0;
}
