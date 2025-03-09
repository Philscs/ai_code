#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_JOBS 32

// Struktur für einen Job
typedef struct {
    pid_t pid;           // Prozess-ID
    char *command;       // Ursprünglicher Befehl
    int status;         // Status (running, stopped, done)
    int job_id;         // Job-Nummer
} Job;

// Globale Variablen
Job jobs[MAX_JOBS];
int job_count = 0;

// Funktionsprototypen
void init_shell(void);
void signal_handler(int signo);
void parse_command(char *line, char **args, int *arg_count);
void execute_command(char **args, int arg_count, int background);
void execute_pipeline(char ***commands, int pipeline_count);
void add_job(pid_t pid, char *command);
void remove_job(int job_id);
void list_jobs(void);
void update_job_status(void);

int main(void) {
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    int arg_count;

    init_shell();

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        // Eingabe lesen
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            break;
        }

        // Newline entfernen
        line[strlen(line) - 1] = '\0';

        // Befehl parsen
        parse_command(line, args, &arg_count);

        if (arg_count == 0) {
            continue;
        }

        // Prüfen auf Hintergrundprozess
        int background = 0;
        if (strcmp(args[arg_count-1], "&") == 0) {
            background = 1;
            args[--arg_count] = NULL;
        }

        // Interne Befehle
        if (strcmp(args[0], "exit") == 0) {
            break;
        } else if (strcmp(args[0], "jobs") == 0) {
            list_jobs();
            continue;
        } else if (strcmp(args[0], "fg") == 0) {
            // Implementierung für Vordergrund-Job
            continue;
        } else if (strcmp(args[0], "bg") == 0) {
            // Implementierung für Hintergrund-Job
            continue;
        }

        // Pipeline prüfen und ausführen
        char ***pipeline_commands = NULL;
        int pipeline_count = 0;
        int is_pipeline = 0;

        for (int i = 0; i < arg_count; i++) {
            if (strcmp(args[i], "|") == 0) {
                is_pipeline = 1;
                break;
            }
        }

        if (is_pipeline) {
            // Pipeline aufbauen und ausführen
            pipeline_commands = malloc(MAX_ARGS * sizeof(char**));
            char **current_command = malloc(MAX_ARGS * sizeof(char*));
            int current_arg = 0;

            for (int i = 0; i < arg_count; i++) {
                if (strcmp(args[i], "|") == 0) {
                    current_command[current_arg] = NULL;
                    pipeline_commands[pipeline_count++] = current_command;
                    current_command = malloc(MAX_ARGS * sizeof(char*));
                    current_arg = 0;
                } else {
                    current_command[current_arg++] = strdup(args[i]);
                }
            }
            current_command[current_arg] = NULL;
            pipeline_commands[pipeline_count++] = current_command;

            execute_pipeline(pipeline_commands, pipeline_count);

            // Speicher freigeben
            for (int i = 0; i < pipeline_count; i++) {
                for (int j = 0; pipeline_commands[i][j] != NULL; j++) {
                    free(pipeline_commands[i][j]);
                }
                free(pipeline_commands[i]);
            }
            free(pipeline_commands);
        } else {
            // Einzelnen Befehl ausführen
            execute_command(args, arg_count, background);
        }

        update_job_status();
    }

    return 0;
}

void init_shell(void) {
    // Signal-Handler einrichten
    signal(SIGCHLD, signal_handler);
    signal(SIGTSTP, signal_handler);
    signal(SIGINT, signal_handler);

    // Shell in eigene Prozessgruppe setzen
    setpgid(0, 0);
}

void signal_handler(int signo) {
    if (signo == SIGCHLD) {
        update_job_status();
    }
}

void parse_command(char *line, char **args, int *arg_count) {
    char *token;
    *arg_count = 0;

    token = strtok(line, " \t");
    while (token != NULL && *arg_count < MAX_ARGS - 1) {
        args[(*arg_count)++] = token;
        token = strtok(NULL, " \t");
    }
    args[*arg_count] = NULL;
}

void execute_command(char **args, int arg_count, int background) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {  // Kindprozess
        // Neue Prozessgruppe erstellen
        setpgid(0, 0);

        if (!background) {
            // Terminal-Kontrolle übernehmen
            tcsetpgrp(STDIN_FILENO, getpid());
        }

        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else {  // Elternprozess
        setpgid(pid, pid);

        if (!background) {
            // Warten auf Vordergrundprozess
            tcsetpgrp(STDIN_FILENO, pid);
            waitpid(pid, NULL, WUNTRACED);
            tcsetpgrp(STDIN_FILENO, getpid());
        } else {
            // Job zur Liste hinzufügen
            add_job(pid, args[0]);
        }
    }
}

void execute_pipeline(char ***commands, int pipeline_count) {
    int pipes[MAX_ARGS][2];
    pid_t pids[MAX_ARGS];

    // Pipes erstellen
    for (int i = 0; i < pipeline_count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    // Prozesse erstellen und verbinden
    for (int i = 0; i < pipeline_count; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("fork");
            return;
        }

        if (pids[i] == 0) {  // Kindprozess
            // Pipe-Verbindungen einrichten
            if (i > 0) {  // Nicht erster Prozess
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if (i < pipeline_count - 1) {  // Nicht letzter Prozess
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Unbenutzte Pipe-Enden schließen
            for (int j = 0; j < pipeline_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(commands[i][0], commands[i]);
            perror("execvp");
            exit(1);
        }
    }

    // Elternprozess: Alle Pipe-Enden schließen
    for (int i = 0; i < pipeline_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Auf alle Kindprozesse warten
    for (int i = 0; i < pipeline_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

void add_job(pid_t pid, char *command) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        jobs[job_count].command = strdup(command);
        jobs[job_count].status = 1;  // running
        jobs[job_count].job_id = job_count + 1;
        job_count++;
    }
}

void remove_job(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            free(jobs[i].command);
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j+1];
            }
            job_count--;
            break;
        }
    }
}

void list_jobs(void) {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s %s\n", 
            jobs[i].job_id,
            jobs[i].pid,
            jobs[i].status == 1 ? "Running" : "Stopped",
            jobs[i].command);
    }
}

void update_job_status(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid) {
                if (WIFSTOPPED(status)) {
                    jobs[i].status = 0;  // stopped
                } else {
                    remove_job(jobs[i].job_id);
                }
                break;
            }
        }
    }
}