#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// Logging Funktion
void log_message(const char *message) {
    FILE *log_file = fopen("mmap_handler.log", "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    } else {
        perror("Logdatei konnte nicht geöffnet werden");
    }
}

// Memory Mapped File Handler Struktur
typedef struct {
    int fd;
    void *mapped_region;
    size_t size;
} MemoryMappedFile;

// Initialisierung des Memory Mapped File Handlers
MemoryMappedFile* mmap_file_init(const char *filepath, size_t size, int create_new) {
    MemoryMappedFile *mmap_file = malloc(sizeof(MemoryMappedFile));
    if (!mmap_file) {
        log_message("Fehler: Speicher konnte nicht allokiert werden.");
        return NULL;
    }

    int flags = O_RDWR;
    if (create_new) {
        flags |= O_CREAT;
    }

    mmap_file->fd = open(filepath, flags, 0666);
    if (mmap_file->fd < 0) {
        log_message("Fehler: Datei konnte nicht geöffnet werden.");
        free(mmap_file);
        return NULL;
    }

    if (create_new && ftruncate(mmap_file->fd, size) == -1) {
        log_message("Fehler: Größe der Datei konnte nicht geändert werden.");
        close(mmap_file->fd);
        free(mmap_file);
        return NULL;
    }

    mmap_file->size = size;
    mmap_file->mapped_region = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mmap_file->fd, 0);
    if (mmap_file->mapped_region == MAP_FAILED) {
        log_message("Fehler: mmap fehlgeschlagen.");
        close(mmap_file->fd);
        free(mmap_file);
        return NULL;
    }

    log_message("Memory Mapped File erfolgreich initialisiert.");
    return mmap_file;
}

// Schreibschutz aktivieren
int mmap_set_readonly(MemoryMappedFile *mmap_file) {
    if (mprotect(mmap_file->mapped_region, mmap_file->size, PROT_READ) == -1) {
        log_message("Fehler: Schreibschutz konnte nicht aktiviert werden.");
        return -1;
    }
    log_message("Schreibschutz erfolgreich aktiviert.");
    return 0;
}

// Cleanup-Routine
void mmap_file_cleanup(MemoryMappedFile *mmap_file) {
    if (!mmap_file) return;

    if (munmap(mmap_file->mapped_region, mmap_file->size) == -1) {
        log_message("Fehler: mmap Speicher konnte nicht freigegeben werden.");
    }

    if (close(mmap_file->fd) == -1) {
        log_message("Fehler: Datei konnte nicht geschlossen werden.");
    }

    free(mmap_file);
    log_message("Memory Mapped File Handler erfolgreich bereinigt.");
}

// Beispiel für die Nutzung
int main() {
    const char *filepath = "example_file.dat";
    size_t size = 1024; // 1 KB

    MemoryMappedFile *mmap_file = mmap_file_init(filepath, size, 1);
    if (!mmap_file) {
        fprintf(stderr, "Fehler bei der Initialisierung des Memory Mapped File Handlers.\n");
        return EXIT_FAILURE;
    }

    // Beispiel-Daten schreiben
    snprintf((char*)mmap_file->mapped_region, size, "Beispieldaten");

    // Schreibschutz aktivieren
    if (mmap_set_readonly(mmap_file) == -1) {
        fprintf(stderr, "Fehler beim Setzen des Schreibschutzes.\n");
    }

    // Cleanup durchführen
    mmap_file_cleanup(mmap_file);

    return EXIT_SUCCESS;
}
