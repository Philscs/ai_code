import queue
import threading
import time
from typing import Dict, List

class Job:
    def __init__(self, name: str, priority: int, timeout: int, args: dict):
        self.name = name
        self.priority = priority
        self.timeout = timeout
        self.args = args

    def serialize(self) -> str:
        return f"{self.name}:{self.priority}:{self.timeout}"

class JobQueueManager:
    def __init__(self):
        # Prioritäts-Heap
        self.job_queue = queue.PriorityQueue()
        # Ressourcenlimits
        self.resource_limits = {}
        # Timeout-Eingabe
        self.timeout = 300  # 5 Minuten

    def add_job(self, job: Job, resource_limit: int):
        # Sicherheitsprüfung, um Command Injection zu verhindern
        if "cmd" in job.args:
            raise ValueError("Command injection not allowed")
        # Job hinzufügen und Prioritäts-Heap aktualisieren
        self.job_queue.put((job.priority + 10 * resource_limit, job))
        # Ressourcenlimits aktualisieren
        self.resource_limits[job.name] = resource_limit

    def run_job(self):
        try:
            # Job aus der Prioritäts-Heap extrahieren und führen
            priority, job = self.job_queue.get(False)
            if time.time() - job.timeout > 0:
                raise Exception("Job über Timeout")
            self.execute_job(job)
            # Ressourcenlimits aktualisieren
            resource_limit = self.resource_limits[job.name]
            resource_limit -= 1
            if resource_limit <= 0:
                del self.resource_limits[job.name]
        except Exception as e:
            print(f"Job {job.name} führte zu einem Fehler: {str(e)}")

    def execute_job(self, job):
        # Job-Argument-Handling
        args = {}
        for arg_key, value in job.args.items():
            if isinstance(value, str) and value.startswith("$("):
                try:
                    # Command Injection Sicherheit
                    cmd_args = [arg.strip('$()').split('=')[-1] for arg in value.split(',')]
                    for i, cmd_arg in enumerate(cmd_args):
                        args[arg_key + f"_{i}"] = self.resource_limits[f"{job.name}_{cmd_arg[0]}"]
                except ValueError:
                    raise ValueError("Invalid command injection syntax")
            else:
                args[arg_key] = value
        # Job-Argument-Ersetzen
        for arg_key, value in args.items():
            job.args[arg_key] = value

    def start(self):
        # Anzahl der verfügbaren Ressourcen setzten
        self.resource_limits = {"job_1": 10}
        threading.Thread(target=self.run_job).start()

# Beispiel-Benutzung:
if __name__ == "__main__":
    job_manager = JobQueueManager()
    # Job hinzufügen und Prioritäts-Heap aktualisieren
    job_manager.add_job(Job("Job1", 1, 300, {"cmd": "ls -l"}), 10)
    job_manager.add_job(Job("Job2", 2, 600, {"cmd": "echo Hello World!"}), 20)
    # Job-Queue starten und Job-Erstellung überwachen
    while True:
        time.sleep(5)
