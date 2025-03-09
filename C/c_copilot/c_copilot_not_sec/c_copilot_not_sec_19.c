#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Task structure
typedef struct {
    int id;
    int priority;
    int deadline;
    int execution_time;
    int remaining_time;
    bool is_running;
} Task;

// Function to schedule tasks
void schedule_tasks(Task tasks[], int num_tasks) {
    // Sort tasks based on priority (higher priority first)
    for (int i = 0; i < num_tasks - 1; i++) {
        for (int j = 0; j < num_tasks - i - 1; j++) {
            if (tasks[j].priority < tasks[j + 1].priority) {
                Task temp = tasks[j];
                tasks[j] = tasks[j + 1];
                tasks[j + 1] = temp;
            }
        }
    }

    // Run tasks based on priority and deadlines
    for (int i = 0; i < num_tasks; i++) {
        Task* current_task = &tasks[i];
        current_task->is_running = true;

        // Execute the task
        printf("Running Task %d\n", current_task->id);
        current_task->remaining_time -= current_task->execution_time;

        // Check if the task missed its deadline
        if (current_task->remaining_time > 0) {
            printf("Task %d missed its deadline\n", current_task->id);
        }

        current_task->is_running = false;
    }
}

int main() {
    // Create an array of tasks
    Task tasks[3] = {
        {1, 2, 10, 5, 5, false},
        {2, 1, 8, 3, 3, false},
        {3, 3, 12, 4, 4, false}
    };

    // Schedule the tasks
    schedule_tasks(tasks, 3);

    return 0;
}
