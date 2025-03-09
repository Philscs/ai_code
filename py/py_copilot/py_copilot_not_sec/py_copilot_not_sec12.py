class Task:
    def __init__(self, name, priority, dependencies):
        self.name = name
        self.priority = priority
        self.dependencies = dependencies
        self.completed = False

class TaskScheduler:
    def __init__(self):
        self.tasks = []

    def add_task(self, task):
        self.tasks.append(task)

    def get_next_task(self):
        available_tasks = [task for task in self.tasks if not task.completed and all(dep.completed for dep in task.dependencies)]
        if available_tasks:
            return max(available_tasks, key=lambda task: task.priority)
        return None

    def mark_task_completed(self, task_name):
        task = next((task for task in self.tasks if task.name == task_name), None)
        if task:
            task.completed = True

# Example usage
scheduler = TaskScheduler()

# Create tasks
task1 = Task("Task 1", 1, [])
task2 = Task("Task 2", 2, [task1])
task3 = Task("Task 3", 3, [task1, task2])

# Add tasks to scheduler
scheduler.add_task(task1)
scheduler.add_task(task2)
scheduler.add_task(task3)

# Get and mark tasks as completed
next_task = scheduler.get_next_task()
while next_task:
    print("Next task:", next_task.name)
    scheduler.mark_task_completed(next_task.name)
    next_task = scheduler.get_next_task()
