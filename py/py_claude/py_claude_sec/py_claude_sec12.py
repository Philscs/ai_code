import logging
import schedule
import threading
import time
from datetime import datetime
from typing import Dict, List, Optional, Callable
from cryptography.fernet import Fernet
import json
import hashlib
import re
from dataclasses import dataclass
from queue import PriorityQueue
import sqlite3
from contextlib import contextmanager

# Konfiguration des Loggings
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class TaskCredentials:
    username: str
    encrypted_password: bytes
    
class TaskSecurityManager:
    def __init__(self, encryption_key: bytes = None):
        """Initialisiert den Security Manager mit einem Verschlüsselungsschlüssel."""
        if encryption_key is None:
            encryption_key = Fernet.generate_key()
        self.cipher_suite = Fernet(encryption_key)
        
    def encrypt_credentials(self, password: str) -> bytes:
        """Verschlüsselt sensitive Daten."""
        return self.cipher_suite.encrypt(password.encode())
    
    def decrypt_credentials(self, encrypted_data: bytes) -> str:
        """Entschlüsselt sensitive Daten."""
        return self.cipher_suite.decrypt(encrypted_data).decode()
    
    def validate_task_input(self, task_input: str) -> bool:
        """Überprüft Task-Input auf potenziell gefährliche Befehle."""
        dangerous_patterns = [
            r"rm\s+-rf",
            r";\s*delete\s+from",
            r";\s*drop\s+table",
            r"eval\(",
            r"exec\(",
            r"system\(",
        ]
        return not any(re.search(pattern, task_input, re.IGNORECASE) 
                      for pattern in dangerous_patterns)

class Task:
    def __init__(self, 
                 name: str,
                 func: Callable,
                 schedule_time: str,
                 max_retries: int = 3,
                 retry_delay: int = 300,
                 dependencies: List[str] = None,
                 credentials: Optional[TaskCredentials] = None):
        self.name = name
        self.func = func
        self.schedule_time = schedule_time
        self.max_retries = max_retries
        self.retry_delay = retry_delay
        self.dependencies = dependencies or []
        self.credentials = credentials
        self.retry_count = 0
        self.last_run = None
        self.status = "pending"
        
    def __lt__(self, other):
        """Implementierung für PriorityQueue-Vergleiche."""
        return len(self.dependencies) < len(other.dependencies)

class TaskDatabase:
    def __init__(self, db_path: str = "tasks.db"):
        self.db_path = db_path
        self._init_db()
        
    @contextmanager
    def get_connection(self):
        conn = sqlite3.connect(self.db_path)
        try:
            yield conn
        finally:
            conn.close()
            
    def _init_db(self):
        """Initialisiert die Datenbank-Tabellen."""
        with self.get_connection() as conn:
            conn.execute("""
                CREATE TABLE IF NOT EXISTS tasks (
                    name TEXT PRIMARY KEY,
                    schedule_time TEXT,
                    max_retries INTEGER,
                    retry_delay INTEGER,
                    status TEXT,
                    last_run TEXT
                )
            """)
            conn.execute("""
                CREATE TABLE IF NOT EXISTS task_dependencies (
                    task_name TEXT,
                    dependency_name TEXT,
                    FOREIGN KEY (task_name) REFERENCES tasks (name),
                    FOREIGN KEY (dependency_name) REFERENCES tasks (name)
                )
            """)
            conn.commit()

    def save_task(self, task: Task):
        """Speichert Task-Informationen in der Datenbank."""
        with self.get_connection() as conn:
            conn.execute("""
                INSERT OR REPLACE INTO tasks 
                (name, schedule_time, max_retries, retry_delay, status, last_run)
                VALUES (?, ?, ?, ?, ?, ?)
            """, (task.name, task.schedule_time, task.max_retries,
                 task.retry_delay, task.status,
                 task.last_run.isoformat() if task.last_run else None))
            
            # Speichere Abhängigkeiten
            conn.execute("DELETE FROM task_dependencies WHERE task_name = ?",
                        (task.name,))
            for dep in task.dependencies:
                conn.execute("""
                    INSERT INTO task_dependencies (task_name, dependency_name)
                    VALUES (?, ?)
                """, (task.name, dep))
            conn.commit()

class TaskScheduler:
    def __init__(self):
        self.tasks: Dict[str, Task] = {}
        self.security_manager = TaskSecurityManager()
        self.task_db = TaskDatabase()
        self.task_queue = PriorityQueue()
        self._stop_flag = threading.Event()
        
    def add_task(self, 
                 name: str,
                 func: Callable,
                 schedule_time: str,
                 max_retries: int = 3,
                 retry_delay: int = 300,
                 dependencies: List[str] = None,
                 credentials: Optional[TaskCredentials] = None) -> bool:
        """Fügt einen neuen Task hinzu."""
        # Validiere Task-Input
        if not self.security_manager.validate_task_input(str(func)):
            logger.error(f"Potenziell gefährlicher Task-Input erkannt: {name}")
            return False
            
        # Überprüfe zirkuläre Abhängigkeiten
        if dependencies and not self._validate_dependencies(name, dependencies):
            logger.error(f"Zirkuläre Abhängigkeit erkannt für Task: {name}")
            return False
            
        task = Task(name, func, schedule_time, max_retries, retry_delay,
                   dependencies, credentials)
        self.tasks[name] = task
        self.task_db.save_task(task)
        self.task_queue.put(task)
        
        return True
        
    def _validate_dependencies(self, task_name: str,
                             dependencies: List[str],
                             visited: set = None) -> bool:
        """Überprüft auf zirkuläre Abhängigkeiten."""
        if visited is None:
            visited = set()
            
        if task_name in visited:
            return False
            
        visited.add(task_name)
        
        for dep in dependencies:
            if dep in self.tasks:
                if not self._validate_dependencies(dep,
                                                self.tasks[dep].dependencies,
                                                visited.copy()):
                    return False
        
        return True
        
    def _can_run_task(self, task: Task) -> bool:
        """Überprüft, ob alle Abhängigkeiten erfüllt sind."""
        for dep_name in task.dependencies:
            dep_task = self.tasks.get(dep_name)
            if not dep_task or dep_task.status != "completed":
                return False
        return True
        
    def _execute_task(self, task: Task):
        """Führt einen Task aus und behandelt Fehler."""
        if not self._can_run_task(task):
            logger.info(f"Task {task.name} wartet auf Abhängigkeiten")
            self.task_queue.put(task)
            return
            
        try:
            # Wenn Credentials benötigt werden, entschlüssele sie
            if task.credentials:
                password = self.security_manager.decrypt_credentials(
                    task.credentials.encrypted_password
                )
                task.func(task.credentials.username, password)
            else:
                task.func()
                
            task.status = "completed"
            task.last_run = datetime.now()
            self.task_db.save_task(task)
            logger.info(f"Task {task.name} erfolgreich ausgeführt")
            
        except Exception as e:
            logger.error(f"Fehler bei Ausführung von Task {task.name}: {str(e)}")
            task.retry_count += 1
            
            if task.retry_count < task.max_retries:
                logger.info(f"Plane Wiederholung für Task {task.name} in "
                          f"{task.retry_delay} Sekunden")
                task.status = "retry"
                threading.Timer(task.retry_delay,
                              lambda: self.task_queue.put(task)).start()
            else:
                logger.error(f"Task {task.name} endgültig fehlgeschlagen nach "
                           f"{task.max_retries} Versuchen")
                task.status = "failed"
            
            self.task_db.save_task(task)
            
    def start(self):
        """Startet den Task-Scheduler."""
        def scheduler_loop():
            while not self._stop_flag.is_set():
                if not self.task_queue.empty():
                    task = self.task_queue.get()
                    self._execute_task(task)
                time.sleep(1)
                
        self.scheduler_thread = threading.Thread(target=scheduler_loop)
        self.scheduler_thread.start()
        
    def stop(self):
        """Stoppt den Task-Scheduler sicher."""
        self._stop_flag.set()
        self.scheduler_thread.join()

# Beispiel-Verwendung
if __name__ == "__main__":
    # Beispiel-Tasks definieren
    def task1():
        print("Task 1 ausgeführt")
        
    def task2(username, password):
        print(f"Task 2 ausgeführt mit Benutzer {username}")
        
    # Scheduler initialisieren
    scheduler = TaskScheduler()
    
    # Tasks hinzufügen
    credentials = TaskCredentials(
        username="test_user",
        encrypted_password=scheduler.security_manager.encrypt_credentials("pass123")
    )
    
    scheduler.add_task(
        name="task1",
        func=task1,
        schedule_time="*/5 * * * *"  # Alle 5 Minuten
    )
    
    scheduler.add_task(
        name="task2",
        func=task2,
        schedule_time="0 * * * *",  # Stündlich
        dependencies=["task1"],
        credentials=credentials
    )
    
    # Scheduler starten
    scheduler.start()
    
    try:
        # Lasse den Scheduler für eine Weile laufen
        time.sleep(3600)
    finally:
        # Stelle sicher, dass der Scheduler sauber beendet wird
        scheduler.stop()