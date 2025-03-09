#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define BLOCK_SIZE 4096
#define MAX_BLOCKS 1024
#define MAX_FILES 256
#define MAX_FILENAME 32
#define MAX_PATH 256
#define JOURNAL_SIZE 100

// Strukturen für das Dateisystem

// FAT-Eintrag
typedef struct {
    int next_block;  // Index des nächsten Blocks oder -1 für Ende
    bool is_used;    // Gibt an ob der Block belegt ist
} FATEntry;

// Verzeichniseintrag
typedef struct {
    char name[MAX_FILENAME];
    int first_block;         // Erster Block der Datei
    size_t size;            // Dateigröße
    time_t created;         // Erstellungszeitpunkt
    time_t modified;        // Letzter Änderungszeitpunkt
    bool is_directory;      // Gibt an ob es sich um ein Verzeichnis handelt
    int parent_dir;         // Index des übergeordneten Verzeichnisses
} DirectoryEntry;

// Journal-Eintrag
typedef struct {
    enum {CREATE, DELETE, MODIFY} operation;
    char path[MAX_PATH];
    time_t timestamp;
} JournalEntry;

// Hauptstruktur des Dateisystems
typedef struct {
    FATEntry fat[MAX_BLOCKS];                    // File Allocation Table
    DirectoryEntry directory[MAX_FILES];         // Verzeichnisstruktur
    char blocks[MAX_BLOCKS][BLOCK_SIZE];         // Datenblöcke
    JournalEntry journal[JOURNAL_SIZE];          // Journal für Operationen
    int journal_index;                           // Aktueller Index im Journal
    int root_directory;                          // Index des Root-Verzeichnisses
} FileSystem;

// Funktionen für die Verwaltung des Dateisystems

// Initialisiert ein neues Dateisystem
FileSystem* init_filesystem() {
    FileSystem* fs = (FileSystem*)malloc(sizeof(FileSystem));
    if (!fs) return NULL;

    // Initialisiere FAT
    for (int i = 0; i < MAX_BLOCKS; i++) {
        fs->fat[i].next_block = -1;
        fs->fat[i].is_used = false;
    }

    // Initialisiere Verzeichniseinträge
    for (int i = 0; i < MAX_FILES; i++) {
        fs->directory[i].first_block = -1;
        fs->directory[i].size = 0;
        fs->directory[i].is_directory = false;
        fs->directory[i].parent_dir = -1;
    }

    // Erstelle Root-Verzeichnis
    fs->root_directory = 0;
    strcpy(fs->directory[0].name, "/");
    fs->directory[0].is_directory = true;
    fs->directory[0].parent_dir = -1;
    fs->directory[0].created = time(NULL);
    fs->directory[0].modified = time(NULL);

    // Initialisiere Journal
    fs->journal_index = 0;

    return fs;
}

// Findet einen freien Block in der FAT
int find_free_block(FileSystem* fs) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!fs->fat[i].is_used) {
            return i;
        }
    }
    return -1;
}

// Findet einen freien Verzeichniseintrag
int find_free_directory_entry(FileSystem* fs) {
    for (int i = 1; i < MAX_FILES; i++) {  // Start bei 1, da 0 das Root-Verzeichnis ist
        if (fs->directory[i].first_block == -1) {
            return i;
        }
    }
    return -1;
}

// Fügt einen Eintrag zum Journal hinzu
void add_journal_entry(FileSystem* fs, int operation, const char* path) {
    JournalEntry* entry = &fs->journal[fs->journal_index];
    entry->operation = operation;
    strncpy(entry->path, path, MAX_PATH - 1);
    entry->timestamp = time(NULL);
    
    fs->journal_index = (fs->journal_index + 1) % JOURNAL_SIZE;
}

// Erstellt eine neue Datei
int create_file(FileSystem* fs, const char* path, bool is_directory) {
    // Finde freien Verzeichniseintrag
    int entry_index = find_free_directory_entry(fs);
    if (entry_index == -1) return -1;

    // Extrahiere Dateinamen und Pfad
    char filename[MAX_FILENAME];
    char parent_path[MAX_PATH];
    
    // Hole den letzten Teil des Pfads als Dateinamen
    const char* last_slash = strrchr(path, '/');
    if (last_slash) {
        strncpy(filename, last_slash + 1, MAX_FILENAME - 1);
        size_t parent_len = last_slash - path;
        strncpy(parent_path, path, parent_len);
        parent_path[parent_len] = '\0';
    } else {
        strncpy(filename, path, MAX_FILENAME - 1);
        strcpy(parent_path, "/");
    }

    // Erstelle den Verzeichniseintrag
    DirectoryEntry* entry = &fs->directory[entry_index];
    strncpy(entry->name, filename, MAX_FILENAME - 1);
    entry->first_block = -1;  // Noch keine Datenblöcke zugewiesen
    entry->size = 0;
    entry->created = time(NULL);
    entry->modified = time(NULL);
    entry->is_directory = is_directory;
    
    // Füge Journal-Eintrag hinzu
    add_journal_entry(fs, CREATE, path);

    return entry_index;
}

// Schreibt Daten in eine Datei
int write_file(FileSystem* fs, int file_index, const void* data, size_t size) {
    if (file_index < 0 || file_index >= MAX_FILES) return -1;
    if (fs->directory[file_index].is_directory) return -1;

    DirectoryEntry* entry = &fs->directory[file_index];
    
    // Berechne benötigte Anzahl an Blöcken
    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    // Allokiere Blöcke
    int current_block = -1;
    int first_block = -1;
    const char* data_ptr = (const char*)data;
    
    for (int i = 0; i < blocks_needed; i++) {
        int new_block = find_free_block(fs);
        if (new_block == -1) {
            // Nicht genug freie Blöcke
            return -1;
        }
        
        if (first_block == -1) {
            first_block = new_block;
        }
        
        if (current_block != -1) {
            fs->fat[current_block].next_block = new_block;
        }
        
        fs->fat[new_block].is_used = true;
        fs->fat[new_block].next_block = -1;
        
        // Kopiere Daten in den Block
        size_t block_size = (i == blocks_needed - 1) ? 
            size - (i * BLOCK_SIZE) : BLOCK_SIZE;
        memcpy(fs->blocks[new_block], data_ptr + (i * BLOCK_SIZE), block_size);
        
        current_block = new_block;
    }
    
    entry->first_block = first_block;
    entry->size = size;
    entry->modified = time(NULL);
    
    // Füge Journal-Eintrag hinzu
    add_journal_entry(fs, MODIFY, entry->name);
    
    return 0;
}

// Liest Daten aus einer Datei
int read_file(FileSystem* fs, int file_index, void* buffer, size_t size) {
    if (file_index < 0 || file_index >= MAX_FILES) return -1;
    if (fs->directory[file_index].is_directory) return -1;

    DirectoryEntry* entry = &fs->directory[file_index];
    if (size > entry->size) size = entry->size;

    char* buf_ptr = (char*)buffer;
    int current_block = entry->first_block;
    size_t bytes_read = 0;

    while (current_block != -1 && bytes_read < size) {
        size_t block_size = (size - bytes_read < BLOCK_SIZE) ? 
            size - bytes_read : BLOCK_SIZE;
        
        memcpy(buf_ptr + bytes_read, fs->blocks[current_block], block_size);
        bytes_read += block_size;
        
        current_block = fs->fat[current_block].next_block;
    }

    return bytes_read;
}

// Löscht eine Datei oder ein Verzeichnis
int delete_file(FileSystem* fs, int file_index) {
    if (file_index < 0 || file_index >= MAX_FILES) return -1;
    if (file_index == fs->root_directory) return -1;  // Root kann nicht gelöscht werden

    DirectoryEntry* entry = &fs->directory[file_index];
    
    // Füge Journal-Eintrag hinzu
    add_journal_entry(fs, DELETE, entry->name);

    // Gebe alle Blöcke frei
    int current_block = entry->first_block;
    while (current_block != -1) {
        int next_block = fs->fat[current_block].next_block;
        fs->fat[current_block].is_used = false;
        fs->fat[current_block].next_block = -1;
        current_block = next_block;
    }

    // Lösche Verzeichniseintrag
    entry->first_block = -1;
    entry->size = 0;
    entry->name[0] = '\0';
    
    return 0;
}

// Beispiel für die Verwendung
int main() {
    // Initialisiere Dateisystem
    FileSystem* fs = init_filesystem();
    if (!fs) {
        printf("Fehler beim Initialisieren des Dateisystems\n");
        return 1;
    }

    // Erstelle ein Verzeichnis
    int dir_index = create_file(fs, "/dokumente", true);
    printf("Verzeichnis erstellt: %d\n", dir_index);

    // Erstelle eine Datei
    int file_index = create_file(fs, "/dokumente/test.txt", false);
    printf("Datei erstellt: %d\n", file_index);

    // Schreibe Daten in die Datei
    const char* test_data = "Hello, File System!";
    write_file(fs, file_index, test_data, strlen(test_data) + 1);

    // Lese Daten aus der Datei
    char buffer[100];
    int bytes_read = read_file(fs, file_index, buffer, sizeof(buffer));
    printf("Gelesene Daten: %s\n", buffer);

    // Lösche die Datei
    delete_file(fs, file_index);

    // Gebe Ressourcen frei
    free(fs);

    return 0;
}