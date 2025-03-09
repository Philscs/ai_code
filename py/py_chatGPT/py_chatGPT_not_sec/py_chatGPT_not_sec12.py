import heapq
from collections import defaultdict, deque

class Task:
    def __init__(self, task_id, priority, required_resources, dependencies=None):
        self.task_id = task_id
        self.priority = priority
        self.required_resources = required_resources
        self.dependencies = dependencies if dependencies else []

    def __lt__(self, other):
        return self.priority > other.priority  # Höhere Priorität zuerst

class TaskScheduler:
    def __init__(self, total_resources):
        self.tasks = {}
        self.dependency_graph = defaultdict(list)
        self.in_degree = defaultdict(int)
        self.total_resources = total_resources
        self.available_resources = total_resources

    def add_task(self, task):
        if task.task_id in self.tasks:
            raise ValueError(f"Task {task.task_id} existiert bereits.")
        
        self.tasks[task.task_id] = task
        self.in_degree[task.task_id] = 0
        
        for dep in task.dependencies:
            self.dependency_graph[dep].append(task.task_id)
            self.in_degree[task.task_id] += 1

    def schedule(self):
        # Topologisches Sortieren der Tasks
        ready_queue = []
        for task_id, degree in self.in_degree.items():
            if degree == 0:
                heapq.heappush(ready_queue, self.tasks[task_id])

        execution_order = []

        while ready_queue:
            current_task = heapq.heappop(ready_queue)

            if self.available_resources >= current_task.required_resources:
                # Ressourcen reservieren
                self.available_resources -= current_task.required_resources
                execution_order.append(current_task)

                print(f"Task {current_task.task_id} gestartet. Verfügbare Ressourcen: {self.available_resources}")

                # Abhängigkeiten aktualisieren
                for dependent in self.dependency_graph[current_task.task_id]:
                    self.in_degree[dependent] -= 1
                    if self.in_degree[dependent] == 0:
                        heapq.heappush(ready_queue, self.tasks[dependent])

                # Ressourcen nach der Ausführung wieder freigeben
                self.available_resources += current_task.required_resources
                print(f"Task {current_task.task_id} abgeschlossen. Verfügbare Ressourcen: {self.available_resources}")
            else:
                print(f"Task {current_task.task_id} kann nicht gestartet werden. Nicht genug Ressourcen.")

        if len(execution_order) < len(self.tasks):
            print("Es gibt zyklische Abhängigkeiten oder unzureichende Ressourcen, um alle Tasks auszuführen.")
        
        return execution_order

# Beispiel-Nutzung
if __name__ == "__main__":
    scheduler = TaskScheduler(total_resources=10)

    task_a = Task(task_id="A", priority=3, required_resources=4)
    task_b = Task(task_id="B", priority=2, required_resources=3, dependencies=["A"])
    task_c = Task(task_id="C", priority=1, required_resources=5, dependencies=["A"])
    task_d = Task(task_id="D", priority=2, required_resources=2, dependencies=["B", "C"])

    scheduler.add_task(task_a)
    scheduler.add_task(task_b)
    scheduler.add_task(task_c)
    scheduler.add_task(task_d)

    scheduler.schedule()
