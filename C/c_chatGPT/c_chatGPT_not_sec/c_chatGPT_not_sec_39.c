#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100
#define MAX_JOBS 10

typedef struct {
    pid_t pid;
    char command[MAX_INPUT];
    int running;
} Job;

Job jobs[MAX_JOBS];
int job_count = 0;

void add_job(pid_t pid, const char *command) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command, command, MAX_INPUT);
        jobs[job_count].running = 1;
        job_count++;
    } else {
        fprintf(stderr, "Maximale Anzahl an Jobs erreicht!\n");
    }
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

void sigchld_handler(int signo) {
    int saved_errno = errno;
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        remove_job(pid);
    }
    errno = saved_errno;
}

void execute_command(char *cmd) {
    char *args[MAX_ARGS];
    int arg_count = 0;

    args[arg_count++] = strtok(cmd, " ");
    while ((args[arg_count++] = strtok(NULL, " ")) != NULL);

    args[arg_count - 1] = NULL;

    if (args[0] == NULL) {
        return;
    }

    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    if (strcmp(args[0], "jobs") == 0) {
        for (int i = 0; i < job_count; i++) {
            printf("[%d] %s\n", jobs[i].pid, jobs[i].command);
        }
        return;
    }

    int background = 0;
    if (strcmp(args[arg_count - 2], "&") == 0) {
        background = 1;
        args[arg_count - 2] = NULL;
    }

    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        if (background) {
            add_job(pid, args[0]);
            printf("Job gestartet: [%d] %s\n", pid, args[0]);
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        perror("fork");
    }
}

void execute_pipeline(char *cmd) {
    char *commands[MAX_ARGS];
    int cmd_count = 0;

    commands[cmd_count++] = strtok(cmd, "|");
    while ((commands[cmd_count++] = strtok(NULL, "|")) != NULL);

    commands[cmd_count - 1] = NULL;

    int pipefd[2];
    int input_fd = 0;

    for (int i = 0; i < cmd_count - 1; i++) {
        pipe(pipefd);

        pid_t pid = fork();
        if (pid == 0) {
            dup2(input_fd, 0);
            if (i < cmd_count - 2) {
                dup2(pipefd[1], 1);
            }
            close(pipefd[0]);

            execute_command(commands[i]);
            exit(0);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
            close(pipefd[1]);
            input_fd = pipefd[0];
        } else {
            perror("fork");
            return;
        }
    }
}

int main() {
    signal(SIGCHLD, sigchld_handler);

    char input[MAX_INPUT];

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strchr(input, '|') != NULL) {
            execute_pipeline(input);
        } else {
            execute_command(input);
        }
    }

    return 0;
}
