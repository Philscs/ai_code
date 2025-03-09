// shared_memory.h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>

#define SHM_SIZE 1024
#define SHM_KEY 12345
#define SEM_KEY 54321
#define MSG_KEY 65432

// Struktur für die Message Queue
struct message {
    long mtype;
    char mtext[100];
};

// Semaphor-Operationen
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Shared Memory Struktur
struct shared_data {
    int counter;
    char message[256];
};

// Semaphor-Funktionen
int init_semaphore(void) {
    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(1);
    }

    union semun argument;
    argument.val = 1;
    if (semctl(semid, 0, SETVAL, argument) == -1) {
        perror("semctl failed");
        exit(1);
    }
    return semid;
}

void semaphore_p(int semid) {
    struct sembuf operations[1];
    operations[0].sem_num = 0;
    operations[0].sem_op = -1;
    operations[0].sem_flg = SEM_UNDO;
    if (semop(semid, operations, 1) == -1) {
        perror("semop P failed");
        exit(1);
    }
}

void semaphore_v(int semid) {
    struct sembuf operations[1];
    operations[0].sem_num = 0;
    operations[0].sem_op = 1;
    operations[0].sem_flg = SEM_UNDO;
    if (semop(semid, operations, 1) == -1) {
        perror("semop V failed");
        exit(1);
    }
}

// Producer Process
void producer(void) {
    // Shared Memory einrichten
    int shmid = shmget(SHM_KEY, sizeof(struct shared_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    struct shared_data *shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Semaphore einrichten
    int semid = init_semaphore();

    // Message Queue einrichten
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget failed");
        exit(1);
    }

    struct message msg;
    msg.mtype = 1;

    while (1) {
        // Kritischen Bereich betreten
        semaphore_p(semid);

        // Shared Memory aktualisieren
        shared_memory->counter++;
        sprintf(shared_memory->message, "Update %d from producer", shared_memory->counter);
        printf("Producer wrote: %s\n", shared_memory->message);

        // Message Queue Nachricht senden
        sprintf(msg.mtext, "Message %d", shared_memory->counter);
        if (msgsnd(msgid, &msg, sizeof(msg.mtext), 0) == -1) {
            perror("msgsnd failed");
            exit(1);
        }

        // Kritischen Bereich verlassen
        semaphore_v(semid);

        sleep(2);  // Kurze Pause
    }

    // Aufräumen
    if (shmdt(shared_memory) == -1) {
        perror("shmdt failed");
        exit(1);
    }
}

// Consumer Process
void consumer(void) {
    // Shared Memory einrichten
    int shmid = shmget(SHM_KEY, sizeof(struct shared_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    struct shared_data *shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Semaphore einrichten
    int semid = init_semaphore();

    // Message Queue einrichten
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget failed");
        exit(1);
    }

    struct message msg;

    while (1) {
        // Kritischen Bereich betreten
        semaphore_p(semid);

        // Shared Memory lesen
        printf("Consumer read: %s\n", shared_memory->message);

        // Message Queue Nachricht empfangen
        if (msgrcv(msgid, &msg, sizeof(msg.mtext), 1, 0) == -1) {
            perror("msgrcv failed");
            exit(1);
        }
        printf("Received message: %s\n", msg.mtext);

        // Kritischen Bereich verlassen
        semaphore_v(semid);

        sleep(1);  // Kurze Pause
    }

    // Aufräumen
    if (shmdt(shared_memory) == -1) {
        perror("shmdt failed");
        exit(1);
    }
}

// Main-Funktion zum Testen
int main(void) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    else if (pid == 0) {
        // Child Process (Consumer)
        consumer();
    }
    else {
        // Parent Process (Producer)
        producer();
    }

    return 0;
}