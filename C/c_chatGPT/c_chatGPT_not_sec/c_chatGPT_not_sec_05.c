#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BLOCK_SIZE 512  // Größe eines Blocks in Bytes
#define NUM_BLOCKS 1024 // Anzahl der Blöcke im Dateisystem
#define MAX_FILENAME 32 // Maximale Länge eines Dateinamens
#define FAT_EOF -1      // Kennzeichnung für das Ende einer Datei in der FAT

// Struktur für Einträge in der FAT
typedef struct {
    int next_block; // Nächster Block in der Kette, -1 für EOF
} FATEntry;

// Struktur für Dateien und Verzeichnisse
typedef struct {
    char name[MAX_FILENAME];
    int start_block; // Startblock der Datei oder des Verzeichnisses
    int size;        // Größe der Datei in Bytes
    int is_directory; // 1 für Verzeichnis, 0 für Datei
} FileEntry;

// Superblock
typedef struct {
    int free_blocks; // Anzahl freier Blöcke
    time_t last_mount_time; // Letzte Einhängezeit
    time_t last_write_time; // Letzte Schreibzeit
} SuperBlock;

// Hauptspeicher des Dateisystems
unsigned char storage[NUM_BLOCKS][BLOCK_SIZE];
FATEntry fat[NUM_BLOCKS];
FileEntry root_directory[NUM_BLOCKS];
SuperBlock superblock;

// Journaling Struktur
typedef struct {
    char operation[16];
    char filename[MAX_FILENAME];
    int block_number;
} JournalEntry;

JournalEntry journal[NUM_BLOCKS];
int journal_index = 0;

// Initialisierung des Dateisystems
void initialize_filesystem() {
    superblock.free_blocks = NUM_BLOCKS;
    superblock.last_mount_time = time(NULL);
    superblock.last_write_time = time(NULL);

    for (int i = 0; i < NUM_BLOCKS; i++) {
        fat[i].next_block = -1; // Alle Blöcke sind zunächst frei
    }

    memset(root_directory, 0, sizeof(root_directory));
}

// Journaling-Funktion
void log_journal(const char *operation, const char *filename, int block_number) {
    strncpy(journal[journal_index].operation, operation, sizeof(journal[journal_index].operation));
    strncpy(journal[journal_index].filename, filename, sizeof(journal[journal_index].filename));
    journal[journal_index].block_number = block_number;
    journal_index++;
}

// Freien Block finden
int find_free_block() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (fat[i].next_block == -1) {
            return i;
        }
    }
    return -1; // Kein freier Block vorhanden
}

// Datei erstellen
int create_file(const char *filename) {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (root_directory[i].start_block == 0) { // Freier Eintrag gefunden
            strncpy(root_directory[i].name, filename, MAX_FILENAME);
            root_directory[i].start_block = find_free_block();
            if (root_directory[i].start_block == -1) {
                printf("Fehler: Kein freier Speicherplatz verfügbar.\n");
                return -1;
            }

            root_directory[i].size = 0;
            root_directory[i].is_directory = 0;

            fat[root_directory[i].start_block].next_block = FAT_EOF;
            superblock.free_blocks--;

            log_journal("CREATE", filename, root_directory[i].start_block);
            return 0;
        }
    }
    printf("Fehler: Kein freier Verzeichniseintrag verfügbar.\n");
    return -1;
}

// Datei lesen
void read_file(const char *filename) {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (strcmp(root_directory[i].name, filename) == 0) {
            int block = root_directory[i].start_block;
            printf("Inhalt von %s:\n", filename);
            while (block != FAT_EOF) {
                fwrite(storage[block], 1, BLOCK_SIZE, stdout);
                block = fat[block].next_block;
            }
            printf("\n");
            return;
        }
    }
    printf("Fehler: Datei %s nicht gefunden.\n", filename);
}

// Datei löschen
int delete_file(const char *filename) {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (strcmp(root_directory[i].name, filename) == 0) {
            int block = root_directory[i].start_block;
            while (block != FAT_EOF) {
                int next_block = fat[block].next_block;
                fat[block].next_block = -1; // Block freigeben
                superblock.free_blocks++;
                block = next_block;
            }

            memset(&root_directory[i], 0, sizeof(FileEntry));
            log_journal("DELETE", filename, -1);
            return 0;
        }
    }
    printf("Fehler: Datei %s nicht gefunden.\n", filename);
    return -1;
}

int main() {
    initialize_filesystem();

    // Beispieloperationen
    create_file("test.txt");
    create_file("demo.txt");
    read_file("test.txt");
    delete_file("test.txt");

    return 0;
}
