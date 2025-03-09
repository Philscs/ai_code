from dataclasses import dataclass
from typing import List, Dict, Set, Optional
from enum import Enum
import heapq
from datetime import datetime

class TaskStatus(Enum):
    PENDING = "PENDING"
    READY = "READY"
    RUNNING = "RUNNING"
    COMPLETED = "COMPLETED"
    FAILED = "FAILED"

@dataclass
class Resource:
    name: str
    capacity: int
    available: int = None
    
    def __post_init__(self):
        if self.available is None:
            self.available = self.capacity

@dataclass
class Task:
    id: str
    name: str
    priority: int
    required_resources: Dict[str, int]
    estimated_duration: int  # in minutes
    dependencies: Set[str] = None
    status: TaskStatus = TaskStatus.PENDING
    start_time: Optional[datetime] = None
    end_time: Optional[datetime] = None
    
    def __post_init__(self):
        if self.dependencies is None:
            self.dependencies = set()
    
    def __lt__(self, other):
        return self.priority > other.priority  # Höhere Priorität zuerst

class TaskScheduler:
    def __init__(self):
        self.tasks: Dict[str, Task] = {}
        self.resources: Dict[str, Resource] = {}
        self.ready_queue = []  # Priority Queue für bereite Tasks
        
    def add_task(self, task: Task) -> None:
        """Fügt einen neuen Task zum Scheduler hinzu."""
        self.tasks[task.id] = task
        self._update_task_status(task)
        
    def add_resource(self, resource: Resource) -> None:
        """Fügt eine neue Ressource zum Scheduler hinzu."""
        self.resources[resource.name] = resource
        
    def _update_task_status(self, task: Task) -> None:
        """Aktualisiert den Status eines Tasks basierend auf seinen Abhängigkeiten."""
        if task.status != TaskStatus.PENDING:
            return
            
        # Prüfe, ob alle Abhängigkeiten erfüllt sind
        dependencies_met = all(
            self.tasks[dep_id].status == TaskStatus.COMPLETED 
            for dep_id in task.dependencies
            if dep_id in self.tasks
        )
        
        if dependencies_met:
            task.status = TaskStatus.READY
            heapq.heappush(self.ready_queue, task)
            
    def _check_resource_availability(self, task: Task) -> bool:
        """Prüft, ob alle benötigten Ressourcen für einen Task verfügbar sind."""
        for resource_name, required_amount in task.required_resources.items():
            if resource_name not in self.resources:
                return False
            if self.resources[resource_name].available < required_amount:
                return False
        return True
        
    def _allocate_resources(self, task: Task) -> None:
        """Weist einem Task die benötigten Ressourcen zu."""
        for resource_name, required_amount in task.required_resources.items():
            self.resources[resource_name].available -= required_amount
            
    def _release_resources(self, task: Task) -> None:
        """Gibt die Ressourcen eines Tasks wieder frei."""
        for resource_name, required_amount in task.required_resources.items():
            self.resources[resource_name].available += required_amount
            
    def schedule_next_task(self) -> Optional[Task]:
        """Plant den nächsten Task ein, der ausgeführt werden kann."""
        while self.ready_queue:
            task = heapq.heappop(self.ready_queue)
            if task.status != TaskStatus.READY:
                continue
                
            if self._check_resource_availability(task):
                task.status = TaskStatus.RUNNING
                task.start_time = datetime.now()
                self._allocate_resources(task)
                return task
                
            # Wenn nicht genug Ressourcen verfügbar sind, Task wieder in Queue einreihen
            heapq.heappush(self.ready_queue, task)
            break
            
        return None
        
    def complete_task(self, task_id: str, success: bool = True) -> None:
        """Markiert einen Task als abgeschlossen und aktualisiert abhängige Tasks."""
        task = self.tasks[task_id]
        task.status = TaskStatus.COMPLETED if success else TaskStatus.FAILED
        task.end_time = datetime.now()
        
        self._release_resources(task)
        
        # Aktualisiere abhängige Tasks
        for dependent_task in self.tasks.values():
            if task_id in dependent_task.dependencies:
                self._update_task_status(dependent_task)
                
    def get_task_status(self, task_id: str) -> TaskStatus:
        """Gibt den aktuellen Status eines Tasks zurück."""
        return self.tasks[task_id].status
        
    def get_resource_usage(self) -> Dict[str, Dict]:
        """Gibt die aktuelle Ressourcenauslastung zurück."""
        return {
            name: {
                "capacity": resource.capacity,
                "available": resource.available,
                "utilization": (resource.capacity - resource.available) / resource.capacity
            }
            for name, resource in self.resources.items()
        }

# Beispielverwendung:
def example_usage():
    scheduler = TaskScheduler()
    
    # Ressourcen hinzufügen
    scheduler.add_resource(Resource("CPU", capacity=4))
    scheduler.add_resource(Resource("RAM", capacity=16))
    
    # Tasks erstellen
    task1 = Task(
        id="task1",
        name="Datenverarbeitung",
        priority=1,
        required_resources={"CPU": 2, "RAM": 4},
        estimated_duration=30
    )
    
    task2 = Task(
        id="task2",
        name="Modelltraining",
        priority=2,
        required_resources={"CPU": 3, "RAM": 8},
        estimated_duration=60,
        dependencies={"task1"}
    )
    
    # Tasks zum Scheduler hinzufügen
    scheduler.add_task(task1)
    scheduler.add_task(task2)
    
    # Ersten Task ausführen
    next_task = scheduler.schedule_next_task()
    if next_task:
        print(f"Starte Task: {next_task.name}")
        # Simulation der Taskausführung...
        scheduler.complete_task(next_task.id)
    
    # Zweiten Task ausführen
    next_task = scheduler.schedule_next_task()
    if next_task:
        print(f"Starte Task: {next_task.name}")
        # Simulation der Taskausführung...
        scheduler.complete_task(next_task.id)

if __name__ == "__main__":
    example_usage()