#include <stdio.h>
#include <stdlib.h>

// Datei-Struktur
typedef struct File {
    char name[20];
    int size;
    int mode; // 0: Lesen, 1: Schreiben, 2: Ausführen
} file_t;

// Ressourcen-Struktur
typedef struct Resource {
    char name[20];
    int usage; // Anzahl der von der Datei genutzten Ressourcen
    int maxUsage; // Maximale Anzahl an Ressourcen, die pro Zeit einstellt werden dürfen
} resource_t;

// Dateisystem-Struktur
typedef struct CFS {
    file_t *files;
    resource_t *resources;
    int numFiles;
    int numResources;
} cfs_t;

// Funktion zum Erstellen eines neuen Dateisystems
cfs_t *createCFS() {
    cfs_t *cfs = (cfs_t *)malloc(sizeof(cfs_t));
    cfs->files = NULL;
    cfs->resources = NULL;
    cfs->numFiles = 0;
    cfs->numResources = 0;

    return cfs;
}

// Funktion zum Hinzufügen eines neuen Dateisystems
void addFile(cfs_t *cfs, char *name) {
    file_t *file = (file_t *)malloc(sizeof(file_t));
    strcpy(file->name, name);
    file->size = 0;
    file->mode = 0; // Default-Wert

    cfs->files = realloc(cfs->files, sizeof(file_t) * (cfs->numFiles + 1));
    cfs->files[cfs->numFiles] = *file;

    cfs->numFiles++;

    printf("Datei '%s' wurde erfolgreich erstellt.\n", name);
}

// Funktion zum Hinzufügen von Ressourcen-Objekten
void addResource(cfs_t *cfs, char *name) {
    resource_t *resource = (resource_t *)malloc(sizeof(resource_t));
    strcpy(resource->name, name);
    resource->usage = 0;
    resource->maxUsage = 10;

    cfs->resources = realloc(cfs->resources, sizeof(resource_t) * (cfs->numResources + 1));
    cfs->resources[cfs->numResources] = *resource;

    cfs->numResources++;

    printf("Ressource '%s' wurde erfolgreich erstellt.\n", name);
}

// Funktion zum Hinzufügen von Ressourcen
void addResourceUsage(cfs_t *cfs, char *name, int usage) {
    for (int i = 0; i < cfs->numResources; i++) {
        if (strcmp(cfs->resources[i].name, name) == 0) {
            cfs->resources[i].usage += usage;

            printf("Ressource '%s' wurde erfolgreich aktualisiert.\n", name);
            return;
        }
    }

    printf("Ressource nicht gefunden.\n");
}

// Funktion zum Überprüfen der Ressourcennutzung
void checkResourceUsage(cfs_t *cfs) {
    for (int i = 0; i < cfs->numResources; i++) {
        if (cfs->resources[i].usage >= cfs->resources[i].maxUsage) {
            printf("Ressource '%s' ist vollgefüllt!\n", cfs->resources[i].name);
        }
    }
}

// Funktion zum Lösen eines Dateisystems
void deleteCFS(cfs_t *cfs) {
    free(cfs->files);
    free(cfs->resources);

    printf("Dateisystem wurde erfolgreich gelöscht.\n");
}

int main() {
    cfs_t *cfs = createCFS();

    addResource(cfs, "ressourc1");
    addResource(cfs, "ressourc2");

    addResourceUsage(cfs, "ressourc1", 5);
    addResourceUsage(cfs, "ressourc1", 10); // Überlastung

    checkResourceUsage(cfs);

    deleteCFS(cfs);

    return 0;
}