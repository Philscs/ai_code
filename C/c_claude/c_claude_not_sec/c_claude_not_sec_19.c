/**
 * Real-time Task Scheduler
 * Features:
 * - Fixed priority preemptive scheduling
 * - Deadline monitoring
 * - Resource management
 * - Priority inheritance
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Constants
#define MAX_TASKS 32
#define MAX_RESOURCES 16

// Task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED
} TaskState;

// Task Control Block (TCB)
typedef struct TCB {
    uint32_t id;
    uint8_t priority;
    uint8_t original_priority;  // For priority inheritance
    uint32_t deadline;
    uint32_t period;
    TaskState state;
    void (*task_function)(void);
    struct TCB* next;
    uint32_t resources;         // Bitmap of held resources
} TCB;

// Resource Control Block (RCB)
typedef struct {
    uint32_t id;
    bool locked;
    TCB* owner;
    TCB* waiting_tasks;        // Tasks waiting for this resource
} RCB;

// Global variables
static TCB* ready_list = NULL;
static TCB* current_task = NULL;
static RCB resources[MAX_RESOURCES];
static uint32_t system_tick = 0;

// Function declarations
void scheduler_init(void);
TCB* create_task(void (*task_function)(void), uint8_t priority, uint32_t deadline, uint32_t period);
void delete_task(TCB* task);
void start_scheduler(void);
void schedule(void);
bool acquire_resource(TCB* task, uint32_t resource_id);
void release_resource(TCB* task, uint32_t resource_id);
void apply_priority_inheritance(TCB* blocking_task, TCB* blocked_task);
void restore_original_priority(TCB* task);

// Initialize the scheduler
void scheduler_init(void) {
    for (int i = 0; i < MAX_RESOURCES; i++) {
        resources[i].id = i;
        resources[i].locked = false;
        resources[i].owner = NULL;
        resources[i].waiting_tasks = NULL;
    }
}

// Create a new task
TCB* create_task(void (*task_function)(void), uint8_t priority, uint32_t deadline, uint32_t period) {
    TCB* new_task = (TCB*)malloc(sizeof(TCB));
    if (new_task == NULL) return NULL;

    static uint32_t next_id = 0;
    new_task->id = next_id++;
    new_task->priority = priority;
    new_task->original_priority = priority;
    new_task->deadline = deadline;
    new_task->period = period;
    new_task->state = TASK_READY;
    new_task->task_function = task_function;
    new_task->resources = 0;
    new_task->next = NULL;

    // Insert into ready list sorted by priority
    if (ready_list == NULL || ready_list->priority < priority) {
        new_task->next = ready_list;
        ready_list = new_task;
    } else {
        TCB* current = ready_list;
        while (current->next != NULL && current->next->priority >= priority) {
            current = current->next;
        }
        new_task->next = current->next;
        current->next = new_task;
    }

    return new_task;
}

// Delete a task
void delete_task(TCB* task) {
    if (task == NULL) return;

    // Remove from ready list
    if (ready_list == task) {
        ready_list = task->next;
    } else {
        TCB* current = ready_list;
        while (current != NULL && current->next != task) {
            current = current->next;
        }
        if (current != NULL) {
            current->next = task->next;
        }
    }

    // Release all held resources
    for (int i = 0; i < MAX_RESOURCES; i++) {
        if (task->resources & (1 << i)) {
            release_resource(task, i);
        }
    }

    free(task);
}

// Main scheduling function
void schedule(void) {
    // Check deadlines of all tasks
    TCB* task = ready_list;
    while (task != NULL) {
        if (task->state == TASK_READY && system_tick >= task->deadline) {
            // Handle deadline miss
            task->deadline += task->period;  // Set next deadline
        }
        task = task->next;
    }

    // Find highest priority ready task
    TCB* highest_priority_task = NULL;
    task = ready_list;
    while (task != NULL) {
        if (task->state == TASK_READY) {
            highest_priority_task = task;
            break;
        }
        task = task->next;
    }

    // Perform context switch if necessary
    if (highest_priority_task != current_task) {
        if (current_task != NULL) {
            current_task->state = TASK_READY;
        }
        current_task = highest_priority_task;
        if (current_task != NULL) {
            current_task->state = TASK_RUNNING;
            current_task->task_function();
        }
    }
}

// Resource acquisition
bool acquire_resource(TCB* task, uint32_t resource_id) {
    if (resource_id >= MAX_RESOURCES) return false;
    RCB* resource = &resources[resource_id];

    if (!resource->locked) {
        // Resource is free
        resource->locked = true;
        resource->owner = task;
        task->resources |= (1 << resource_id);
        return true;
    } else {
        // Resource is locked - implement priority inheritance
        if (resource->owner->priority < task->priority) {
            apply_priority_inheritance(resource->owner, task);
        }
        
        // Add task to waiting list
        task->state = TASK_BLOCKED;
        TCB* waiting = resource->waiting_tasks;
        if (waiting == NULL || waiting->priority < task->priority) {
            task->next = resource->waiting_tasks;
            resource->waiting_tasks = task;
        } else {
            while (waiting->next != NULL && waiting->next->priority >= task->priority) {
                waiting = waiting->next;
            }
            task->next = waiting->next;
            waiting->next = task;
        }
        return false;
    }
}

// Resource release
void release_resource(TCB* task, uint32_t resource_id) {
    if (resource_id >= MAX_RESOURCES) return;
    RCB* resource = &resources[resource_id];

    if (resource->owner == task) {
        task->resources &= ~(1 << resource_id);
        
        // Restore original priority if no other resources held
        if (task->resources == 0) {
            restore_original_priority(task);
        }

        // Wake up highest priority waiting task
        if (resource->waiting_tasks != NULL) {
            TCB* next_task = resource->waiting_tasks;
            resource->waiting_tasks = next_task->next;
            next_task->state = TASK_READY;
            resource->owner = next_task;
            next_task->resources |= (1 << resource_id);
        } else {
            resource->locked = false;
            resource->owner = NULL;
        }
    }
}

// Apply priority inheritance
void apply_priority_inheritance(TCB* blocking_task, TCB* blocked_task) {
    if (blocking_task->priority < blocked_task->priority) {
        blocking_task->priority = blocked_task->priority;
        
        // Re-sort ready list based on new priority
        if (blocking_task->state == TASK_READY) {
            // Remove task from current position
            if (ready_list == blocking_task) {
                ready_list = blocking_task->next;
            } else {
                TCB* current = ready_list;
                while (current->next != blocking_task) {
                    current = current->next;
                }
                current->next = blocking_task->next;
            }
            
            // Re-insert at new position
            if (ready_list == NULL || ready_list->priority < blocking_task->priority) {
                blocking_task->next = ready_list;
                ready_list = blocking_task;
            } else {
                TCB* current = ready_list;
                while (current->next != NULL && 
                       current->next->priority >= blocking_task->priority) {
                    current = current->next;
                }
                blocking_task->next = current->next;
                current->next = blocking_task;
            }
        }
    }
}

// Restore original task priority
void restore_original_priority(TCB* task) {
    uint8_t old_priority = task->priority;
    task->priority = task->original_priority;
    
    // Re-sort in ready list if priority changed and task is ready
    if (old_priority != task->priority && task->state == TASK_READY) {
        // Remove from current position
        if (ready_list == task) {
            ready_list = task->next;
        } else {
            TCB* current = ready_list;
            while (current->next != task) {
                current = current->next;
            }
            current->next = task->next;
        }
        
        // Re-insert at correct position
        if (ready_list == NULL || ready_list->priority < task->priority) {
            task->next = ready_list;
            ready_list = task;
        } else {
            TCB* current = ready_list;
            while (current->next != NULL && 
                   current->next->priority >= task->priority) {
                current = current->next;
            }
            task->next = current->next;
            current->next = task;
        }
    }
}

// Start the scheduler
void start_scheduler(void) {
    while (1) {
        schedule();
        system_tick++;  // Update system time
    }
}

// Example usage
void example_task1(void) {
    // Task implementation
}

void example_task2(void) {
    // Task implementation
}

int main(void) {
    scheduler_init();
    
    // Create some tasks
    TCB* task1 = create_task(example_task1, 1, 100, 100);
    TCB* task2 = create_task(example_task2, 2, 200, 200);
    
    // Start the scheduler
    start_scheduler();
    
    return 0;
}