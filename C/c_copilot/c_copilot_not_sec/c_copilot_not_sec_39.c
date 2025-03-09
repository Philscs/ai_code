#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 10
#define MAX_ARGS 10
#define MAX_COMMAND_LENGTH 100

void execute_command(char *command) {
    char *args[MAX_ARGS];
    int i = 0;

    args[i] = strtok(command, " ");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " ");
    }

    if (execvp(args[0], args) == -1) {
        perror("Error executing command");
        exit(EXIT_FAILURE);
    }
}

void execute_pipeline(char *commands[MAX_COMMANDS], int num_commands) {
    int i;
    int pipefd[2];
    pid_t pid;

    int prev_pipe = 0; // File descriptor for the previous pipe

    for (i = 0; i < num_commands; i++) {
        if (pipe(pipefd) == -1) {
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }

        pid = fork();
        if (pid == -1) {
            perror("Error forking process");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process

            // Redirect input from previous pipe
            if (i > 0) {
                dup2(prev_pipe, STDIN_FILENO);
                close(prev_pipe);
            }

            // Redirect output to current pipe
            if (i < num_commands - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }

            // Execute command
            execute_command(commands[i]);

            exit(EXIT_SUCCESS);
        } else {
            // Parent process

            // Close previous pipe
            if (i > 0) {
                close(prev_pipe);
            }

            // Save current pipe for next iteration
            prev_pipe = pipefd[0];

            // Close unused write end of current pipe
            close(pipefd[1]);
        }
    }

    // Wait for all child processes to finish
    for (i = 0; i < num_commands; i++) {
        wait(NULL);
    }
}

int main() {
    char commands[MAX_COMMANDS][MAX_COMMAND_LENGTH];
    int num_commands = 0;

    printf("Custom Shell Interpreter\n");
    printf("Enter commands (type 'exit' to quit):\n");

    while (1) {
        printf("> ");
        fgets(commands[num_commands], MAX_COMMAND_LENGTH, stdin);

        // Remove newline character from the end of the command
        commands[num_commands][strcspn(commands[num_commands], "\n")] = '\0';

        if (strcmp(commands[num_commands], "exit") == 0) {
            break;
        }

        num_commands++;

        if (num_commands == MAX_COMMANDS) {
            printf("Maximum number of commands reached\n");
            break;
        }
    }

    execute_pipeline(commands, num_commands);

    return 0;
}
