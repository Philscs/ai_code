import datetime
from collections import defaultdict
from typing import List

class Task:
    def __init__(self, name: str, description: str):
        self.name = name
        self.description = description
        self.durition = 0
        self.dependent_on = []

class Scheduler:
    def __init__(self):
        self.tasks = {}

    def add_task(self, task: Task):
        self.tasks[task.name] = task

    def remove_task(self, task_name: str):
        if task_name in self.tasks:
            del self.tasks[task_name]

    def update_durition(self, task_name: str, duration: int):
        if task_name in self.tasks:
            self.tasks[task_name].durition = duration
class Dependencies:
    def __init__(self, scheduler: Scheduler):
        self.scheduler = scheduler

    def add_dependency(self, task_name: str, dependent_task_name: str):
        if task_name in self.scheduler.tasks and dependent_task_name in self.scheduler.tasks:
            self.scheduler.tasks[task_name].dependent_on.append(dependent_task_name)

    def remove_dependency(self, task_name: str, dependent_task_name: str):
        if task_name in self.scheduler.tasks and dependent_task_name in self.scheduler.tasks:
            self.scheduler.tasks[task_name].dependent_on.remove(dependent_task_name)
class Priorities:
    def __init__(self, scheduler: Scheduler):
        self.scheduler = scheduler

    def add_priority(self, task_name: str, priority: int):
        if task_name in self.scheduler.tasks:
            self.scheduler.tasks[task_name].priority = priority

    def remove_priority(self, task_name: str):
        if task_name in self.scheduler.tasks:
            del self.scheduler.tasks[task_name].priority
class Distribution:
    def __init__(self, scheduler: Scheduler):
        self.scheduler = scheduler

    def distribute_tasks(self):
        # Simple Round-Robin-Distribution
        for task in sorted(self.scheduler.tasks.values(), key=lambda x: x.durition):
            available_resources = self.scheduler.get_available_resources()
            if len(available_resources) > 0:
                # Find the first available resource with enough capacity to run the task
                for resource in available_resources:
                    if resource.capacity >= task.durition:
                        resource.tasks.append(task)
                        break

    def get_available_resources(self):
        # Simple resource allocation based on CPU and memory
        resources = []
        total_cpu = 0
        total_memory = 0
        for task in self.scheduler.tasks.values():
            total_cpu += task.durition * 1.0 / (task.durition + 1)
            total_memory += task.durition * 1024
        for resource_type, capacity in [('CPU', 100), ('Memory', 512)]:
            if total_cpu > capacity or total_memory > capacity:
                resources.append({'name': f'{resource_type}-Resource', 'capacity': capacity})
        return resources