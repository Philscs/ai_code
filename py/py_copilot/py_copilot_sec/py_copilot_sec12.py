import datetime
import threading
import queue

class Task:
    def __init__(self, name, func, args=None, kwargs=None, dependencies=None):
        self.name = name
        self.func = func
        self.args = args or []
        self.kwargs = kwargs or {}
        self.dependencies = dependencies or []

class TaskScheduler:
    def __init__(self):
        self.tasks = {}
        self.failed_tasks = set()
        self.task_queue = queue.Queue()
        self.lock = threading.Lock()

    def add_task(self, task):
        with self.lock:
            self.tasks[task.name] = task

    def add_dependency(self, task_name, dependency_name):
        with self.lock:
            if task_name in self.tasks and dependency_name in self.tasks:
                task = self.tasks[task_name]
                dependency = self.tasks[dependency_name]
                task.dependencies.append(dependency)

    def run_task(self, task):
        try:
            task.func(*task.args, **task.kwargs)
        except Exception as e:
            print(f"Task '{task.name}' failed: {e}")
            self.failed_tasks.add(task.name)

    def schedule_task(self, task_name):
        with self.lock:
            if task_name in self.tasks:
                task = self.tasks[task_name]
                if all(dep.name not in self.failed_tasks for dep in task.dependencies):
                    self.task_queue.put(task)

    def run_scheduler(self):
        while True:
            task = self.task_queue.get()
            self.run_task(task)
            self.task_queue.task_done()

    def start(self):
        scheduler_thread = threading.Thread(target=self.run_scheduler)
        scheduler_thread.start()

# Example usage
def task1():
    print("Task 1 executed")

def task2():
    print("Task 2 executed")

def task3():
    print("Task 3 executed")

scheduler = TaskScheduler()

task1 = Task("task1", task1)
task2 = Task("task2", task2, dependencies=[task1])
task3 = Task("task3", task3, dependencies=[task2])

scheduler.add_task(task1)
scheduler.add_task(task2)
scheduler.add_task(task3)

scheduler.start()

scheduler.schedule_task("task3")
