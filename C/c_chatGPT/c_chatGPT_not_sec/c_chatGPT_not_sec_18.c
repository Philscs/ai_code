#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <unistd.h>

#define SHM_SIZE 1024
#define MSG_SIZE 256

// Strukturen für die Message Queue
typedef struct msgbuf {
    long mtype;
    char mtext[MSG_SIZE];
} Message;

// Semaphore-Operationen
void semaphore_lock(int semid) {
    struct sembuf sem_op = {0, -1, 0};
    semop(semid, &sem_op, 1);
}

void semaphore_unlock(int semid) {
    struct sembuf sem_op = {0, 1, 0};
    semop(semid, &sem_op, 1);
}

int main() {
    key_t key = ftok("shmfile", 65);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Gemeinsamer Speicher erstellen
    int shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Semaphore erstellen
    int semid = semget(key, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Initialisieren des Semaphors
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    // Message Queue erstellen
    int msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    // Prozess-Forking für Sender und Empfänger
    if (fork() == 0) {  // Kindprozess: Sender
        char *data = (char *)shmat(shmid, NULL, 0);
        if (data == (char *)-1) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }

        semaphore_lock(semid);
        strncpy(data, "Hello from Sender!", SHM_SIZE);
        printf("Sender: Nachricht in gemeinsamen Speicher geschrieben.\n");
        semaphore_unlock(semid);

        Message msg = {1, "Zusätzliche Nachricht per Queue"};
        if (msgsnd(msqid, &msg, strlen(msg.mtext) + 1, 0) == -1) {
            perror("msgsnd");
        }

        shmdt(data);
        exit(EXIT_SUCCESS);
    } else {  // Elternprozess: Empfänger
        wait(NULL);  // Warten, bis Kindprozess abgeschlossen ist

        char *data = (char *)shmat(shmid, NULL, 0);
        if (data == (char *)-1) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }

        semaphore_lock(semid);
        printf("Empfänger: Nachricht aus gemeinsamem Speicher: %s\n", data);
        semaphore_unlock(semid);

        Message msg;
        if (msgrcv(msqid, &msg, MSG_SIZE, 1, 0) == -1) {
            perror("msgrcv");
        } else {
            printf("Empfänger: Nachricht aus Message Queue: %s\n", msg.mtext);
        }

        shmdt(data);

        // Ressourcen aufräumen
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        msgctl(msqid, IPC_RMID, NULL);
    }

    return 0;
}
