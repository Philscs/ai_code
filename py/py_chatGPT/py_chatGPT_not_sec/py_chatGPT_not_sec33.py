import concurrent.futures
from collections import defaultdict, deque
import time

class BuildSystem:
    def __init__(self):
        self.tasks = {}
        self.dependencies = defaultdict(list)

    def add_task(self, name, func, dependencies=None):
        """Fügt eine Aufgabe zum Build-System hinzu.

        Args:
            name (str): Name der Aufgabe.
            func (callable): Funktion, die die Aufgabe ausführt.
            dependencies (list[str], optional): Liste von Aufgaben, von denen diese abhängt.
        """
        if dependencies is None:
            dependencies = []
        self.tasks[name] = func
        self.dependencies[name].extend(dependencies)

    def _resolve_dependencies(self):
        """Prüft, ob Zyklen in den Abhängigkeiten vorhanden sind, und sortiert die Aufgaben topologisch."""
        in_degree = {task: 0 for task in self.tasks}
        for deps in self.dependencies.values():
            for dep in deps:
                in_degree[dep] += 1

        # Topologische Sortierung
        queue = deque([task for task, degree in in_degree.items() if degree == 0])
        order = []

        while queue:
            task = queue.popleft()
            order.append(task)
            for dependent in self.dependencies[task]:
                in_degree[dependent] -= 1
                if in_degree[dependent] == 0:
                    queue.append(dependent)

        if len(order) != len(self.tasks):
            raise ValueError("Zirkuläre Abhängigkeiten im Build-System erkannt.")

        return order

    def execute(self):
        """Führt alle Aufgaben unter Berücksichtigung von Abhängigkeiten aus."""
        execution_order = self._resolve_dependencies()
        completed = set()
        
        def run_task(task):
            for dep in self.dependencies[task]:
                if dep not in completed:
                    raise RuntimeError(f"Abhängigkeit {dep} für {task} wurde nicht abgeschlossen.")
            print(f"Starte Aufgabe: {task}")
            self.tasks[task]()
            completed.add(task)
            print(f"Abgeschlossen: {task}")

        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures = {}

            for task in execution_order:
                # Alle Aufgaben mit bereits erfüllten Abhängigkeiten ausführen
                futures[task] = executor.submit(run_task, task)

            # Auf Fertigstellung aller Aufgaben warten
            concurrent.futures.wait(futures.values())

# Beispielverwendung
def task_a():
    time.sleep(1)
    print("Aufgabe A abgeschlossen")

def task_b():
    time.sleep(2)
    print("Aufgabe B abgeschlossen")

def task_c():
    time.sleep(1)
    print("Aufgabe C abgeschlossen")

def task_d():
    time.sleep(1)
    print("Aufgabe D abgeschlossen")

if __name__ == "__main__":
    build_system = BuildSystem()

    build_system.add_task("A", task_a)
    build_system.add_task("B", task_b, dependencies=["A"])
    build_system.add_task("C", task_c, dependencies=["A"])
    build_system.add_task("D", task_d, dependencies=["B", "C"])

    build_system.execute()
