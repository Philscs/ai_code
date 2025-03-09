#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <string.h>

// Struktur für Prozessinformationen
typedef struct {
    pid_t pid;
    double cpu_usage;
    size_t memory_usage;
    int priority;
    uid_t user_id;
} ProcessInfo;

// Globale Variablen für IPC
static int shm_id = -1;
static ProcessInfo* shared_memory = NULL;

// Funktionsdeklarationen
void init_monitor(void);
void cleanup_monitor(void);
int start_monitoring(pid_t pid);
void detect_memory_leaks(pid_t pid);
void drop_privileges(void);

// Signal-Handler
static void signal_handler(int signo) {
    switch (signo) {
        case SIGTERM:
        case SIGINT:
            cleanup_monitor();
            exit(EXIT_SUCCESS);
            break;
        case SIGUSR1:
            // Behandlung benutzerdefinierter Signale
            break;
    }
}

void init_monitor(void) {
    // Initialisierung des Signal-Handlers
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGTERM, &sa, NULL) == -1 ||
        sigaction(SIGINT, &sa, NULL) == -1 ||
        sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Fehler beim Einrichten der Signal-Handler");
        exit(EXIT_FAILURE);
    }

    // Einrichten des Shared Memory für IPC
    key_t key = ftok("/tmp", 'R');
    shm_id = shmget(key, sizeof(ProcessInfo), IPC_CREAT | 0600);
    if (shm_id == -1) {
        perror("Fehler beim Erstellen des Shared Memory");
        exit(EXIT_FAILURE);
    }

    shared_memory = (ProcessInfo*)shmat(shm_id, NULL, 0);
    if (shared_memory == (void*)-1) {
        perror("Fehler beim Anhängen des Shared Memory");
        exit(EXIT_FAILURE);
    }
}

void cleanup_monitor(void) {
    if (shared_memory != NULL) {
        shmdt(shared_memory);
    }
    if (shm_id != -1) {
        shmctl(shm_id, IPC_RMID, NULL);
    }
}

int start_monitoring(pid_t pid) {
    // Überprüfen der Prozess-Existenz
    if (kill(pid, 0) == -1) {
        return -1;
    }

    // Resource Usage Tracking
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == -1) {
        return -1;
    }

    // Aktualisierung der Prozessinformationen im Shared Memory
    shared_memory->pid = pid;
    shared_memory->cpu_usage = usage.ru_utime.tv_sec + usage.ru_stime.tv_sec;
    shared_memory->memory_usage = usage.ru_maxrss;
    
    return 0;
}

void detect_memory_leaks(pid_t pid) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "valgrind --leak-check=full --track-origins=yes -v /proc/%d/exe", pid);
    
    FILE* fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Fehler beim Ausführen von Valgrind");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "definitely lost") || strstr(line, "indirectly lost")) {
            fprintf(stderr, "Speicherleck gefunden: %s", line);
        }
    }

    pclose(fp);
}

void drop_privileges(void) {
    // Wechsel zu einem nicht-privilegierten Benutzer
    struct passwd* nobody = getpwnam("nobody");
    if (nobody == NULL) {
        perror("Fehler beim Abrufen der Nobody-Benutzerinformationen");
        exit(EXIT_FAILURE);
    }

    if (setuid(nobody->pw_uid) == -1) {
        perror("Fehler beim Setzen der Benutzer-ID");
        exit(EXIT_FAILURE);
    }

    // Überprüfen der erfolgreichen Privilegien-Reduzierung
    if (getuid() == 0) {
        fprintf(stderr, "Fehler: Root-Privilegien konnten nicht abgegeben werden\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Verwendung: %s <pid>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Initialisierung
    init_monitor();

    // Privilegien reduzieren
    drop_privileges();

    pid_t target_pid = atoi(argv[1]);
    
    // Hauptüberwachungsschleife
    while (1) {
        if (start_monitoring(target_pid) == -1) {
            fprintf(stderr, "Fehler beim Überwachen von PID %d\n", target_pid);
            break;
        }

        // Speicherlecks überprüfen
        detect_memory_leaks(target_pid);

        // Alle 5 Sekunden aktualisieren
        sleep(5);
    }

    cleanup_monitor();
    return EXIT_SUCCESS;
}