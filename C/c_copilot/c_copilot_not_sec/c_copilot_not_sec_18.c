#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <string.h>

#define SHM_SIZE 1024

// Structure for the message queue
struct message {
    long mtype;
    char mtext[SHM_SIZE];
};

int main() {
    key_t key;
    int shmid, semid, msqid;
    char *shmaddr;
    struct sembuf sem_op;
    struct message msg;

    // Generate a unique key
    key = ftok(".", 'S');

    // Create shared memory segment
    shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory segment
    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    // Create semaphore
    semid = semget(key, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    // Initialize semaphore
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    if (semop(semid, &sem_op, 1) == -1) {
        perror("semop");
        exit(1);
    }

    // Create message queue
    msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("msgget");
        exit(1);
    }

    // Read from shared memory and send message
    strncpy(msg.mtext, shmaddr, SHM_SIZE);
    msg.mtype = 1;
    if (msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    // Receive message from message queue
    if (msgrcv(msqid, &msg, sizeof(msg.mtext), 1, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }

    // Write received message to shared memory
    strncpy(shmaddr, msg.mtext, SHM_SIZE);

    // Detach shared memory segment
    if (shmdt(shmaddr) == -1) {
        perror("shmdt");
        exit(1);
    }

    // Remove shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    // Remove semaphore
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(1);
    }

    // Remove message queue
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }

    return 0;
}
