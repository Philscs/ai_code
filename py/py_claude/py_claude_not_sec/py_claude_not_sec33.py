import asyncio
from dataclasses import dataclass
from enum import Enum
from typing import Dict, List, Set, Callable, Optional
import logging
from concurrent.futures import ThreadPoolExecutor

# Konfiguration des Loggings
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class TaskStatus(Enum):
    """Status eines Build-Tasks"""
    PENDING = "PENDING"
    RUNNING = "RUNNING"
    COMPLETED = "COMPLETED"
    FAILED = "FAILED"

@dataclass
class BuildTask:
    """Repräsentiert einen einzelnen Build-Task"""
    name: str
    action: Callable
    dependencies: Set[str]
    status: TaskStatus = TaskStatus.PENDING
    error: Optional[Exception] = None

class BuildSystem:
    """Hauptklasse zur Verwaltung des Build-Prozesses"""
    def __init__(self):
        self.tasks: Dict[str, BuildTask] = {}
        self.max_workers: int = 4
        
    def add_task(self, name: str, action: Callable, dependencies: Set[str] = None):
        """Fügt einen neuen Task zum Build-System hinzu"""
        if dependencies is None:
            dependencies = set()
        
        # Überprüfe, ob alle Abhängigkeiten existieren
        for dep in dependencies:
            if dep not in self.tasks:
                raise ValueError(f"Abhängigkeit {dep} existiert nicht")
                
        self.tasks[name] = BuildTask(name, action, dependencies)
        logger.info(f"Task {name} hinzugefügt mit Abhängigkeiten: {dependencies}")

    async def _execute_task(self, task: BuildTask) -> None:
        """Führt einen einzelnen Task aus"""
        task.status = TaskStatus.RUNNING
        logger.info(f"Starte Task: {task.name}")
        
        try:
            # Führe den Task in einem ThreadPool aus
            with ThreadPoolExecutor(max_workers=1) as executor:
                await asyncio.get_event_loop().run_in_executor(executor, task.action)
            task.status = TaskStatus.COMPLETED
            logger.info(f"Task erfolgreich abgeschlossen: {task.name}")
        except Exception as e:
            task.status = TaskStatus.FAILED
            task.error = e
            logger.error(f"Fehler in Task {task.name}: {str(e)}")
            raise

    def _get_ready_tasks(self) -> List[str]:
        """Ermittelt Tasks, deren Abhängigkeiten erfüllt sind"""
        ready_tasks = []
        
        for name, task in self.tasks.items():
            if task.status != TaskStatus.PENDING:
                continue
                
            dependencies_met = all(
                self.tasks[dep].status == TaskStatus.COMPLETED
                for dep in task.dependencies
            )
            
            if dependencies_met:
                ready_tasks.append(name)
                
        return ready_tasks

    async def build(self) -> bool:
        """Führt den gesamten Build-Prozess aus"""
        logger.info("Starte Build-Prozess")
        
        # Überprüfe auf zyklische Abhängigkeiten
        if self._has_cycles():
            raise ValueError("Zyklische Abhängigkeiten gefunden")
        
        while True:
            # Finde Tasks, die ausgeführt werden können
            ready_tasks = self._get_ready_tasks()
            
            if not ready_tasks:
                # Prüfe, ob noch Tasks ausstehen
                pending_tasks = [t for t in self.tasks.values() 
                               if t.status == TaskStatus.PENDING]
                if not pending_tasks:
                    break
                    
                # Wenn es noch pending Tasks gibt, aber keine ready sind,
                # muss es einen Fehler geben
                failed_tasks = [t for t in self.tasks.values() 
                              if t.status == TaskStatus.FAILED]
                if failed_tasks:
                    return False
            
            # Führe ready Tasks parallel aus
            tasks = [self._execute_task(self.tasks[name]) 
                    for name in ready_tasks[:self.max_workers]]
            await asyncio.gather(*tasks, return_exceptions=True)
        
        # Prüfe auf fehlgeschlagene Tasks
        failed_tasks = [t for t in self.tasks.values() 
                       if t.status == TaskStatus.FAILED]
        success = len(failed_tasks) == 0
        
        if success:
            logger.info("Build-Prozess erfolgreich abgeschlossen")
        else:
            logger.error(f"Build-Prozess fehlgeschlagen. "
                        f"{len(failed_tasks)} Tasks sind fehlgeschlagen")
            
        return success

    def _has_cycles(self) -> bool:
        """Überprüft auf zyklische Abhängigkeiten im Build-Graph"""
        visited = set()
        path = set()
        
        def visit(task_name: str) -> bool:
            if task_name in path:
                return True
            if task_name in visited:
                return False
                
            visited.add(task_name)
            path.add(task_name)
            
            task = self.tasks[task_name]
            for dep in task.dependencies:
                if visit(dep):
                    return True
                    
            path.remove(task_name)
            return False
        
        return any(visit(task_name) for task_name in self.tasks)

# Beispiel zur Verwendung des Build-Systems
async def example_usage():
    # Erstelle Build-System
    build_system = BuildSystem()
    
    # Definiere einige beispielhafte Build-Tasks
    def compile_code():
        logger.info("Kompiliere Code...")
        # Simuliere Arbeit
        import time
        time.sleep(2)
        
    def run_tests():
        logger.info("Führe Tests aus...")
        time.sleep(1)
        
    def generate_docs():
        logger.info("Generiere Dokumentation...")
        time.sleep(1)
        
    def package():
        logger.info("Erstelle Deployment-Paket...")
        time.sleep(1)
    
    # Füge Tasks mit ihren Abhängigkeiten hinzu
    build_system.add_task("compile", compile_code)
    build_system.add_task("test", run_tests, {"compile"})
    build_system.add_task("docs", generate_docs, {"compile"})
    build_system.add_task("package", package, {"test", "docs"})
    
    # Führe den Build-Prozess aus
    success = await build_system.build()
    
    if success:
        print("Build erfolgreich!")
    else:
        print("Build fehlgeschlagen!")

# Hauptausführung
if __name__ == "__main__":
    asyncio.run(example_usage())