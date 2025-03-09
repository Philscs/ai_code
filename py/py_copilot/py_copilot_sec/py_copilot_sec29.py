import queue
import threading
import time
import json
import subprocess
import shlex
import logging
from dataclasses import dataclass
from typing import Any, Callable, Dict, Optional
from enum import Enum
import pickle
import hashlib
from concurrent.futures import ThreadPoolExecutor

class JobStatus(Enum):
    PENDING = "pending"
    RUNNING = "running"
    COMPLETED = "completed"
    FAILED = "failed"
    TIMEOUT = "timeout"

class JobPriority(Enum):
    LOW = 0
    MEDIUM = 1
    HIGH = 2

@dataclass
class Job:
    id: str
    func: Callable
    args: tuple
    kwargs: dict
    priority: JobPriority
    timeout: int  # Timeout in seconds
    max_retries: int = 3
    retry_count: int = 0
    status: JobStatus = JobStatus.PENDING
    result: Any = None
    error: Optional[str] = None

class SafeSerializer:
    """Sicherer Serialisierer mit Whitelist für erlaubte Typen"""
    
    ALLOWED_TYPES = {
        int, float, str, bool, list, dict, tuple,
        JobStatus, JobPriority
    }

    @classmethod
    def serialize(cls, data: Any) -> bytes:
        """Serialisiert Daten mit Typ-Validierung"""
        if not cls._is_allowed_type(data):
            raise ValueError(f"Nicht erlaubter Typ: {type(data)}")
        
        serialized = pickle.dumps(data)
        return serialized

    @classmethod
    def deserialize(cls, data: bytes) -> Any:
        """Deserialisiert Daten mit Typ-Validierung"""
        obj = pickle.loads(data)
        if not cls._is_allowed_type(obj):
            raise ValueError(f"Nicht erlaubter Typ: {type(obj)}")
        return obj

    @classmethod
    def _is_allowed_type(cls, obj: Any) -> bool:
        """Prüft ob der Typ erlaubt ist"""
        obj_type = type(obj)
        
        if obj_type in cls.ALLOWED_TYPES:
            return True
            
        if obj_type in (list, tuple):
            return all(cls._is_allowed_type(item) for item in obj)
            
        if obj_type is dict:
            return (all(isinstance(k, str) for k in obj.keys()) and
                   all(cls._is_allowed_type(v) for v in obj.values()))
                   
        return False

class ResourceLimiter:
    """Verwaltet Ressourcenlimits für Jobs"""
    
    def __init__(self, max_memory_mb: int, max_cpu_percent: float):
        self.max_memory_mb = max_memory_mb
        self.max_cpu_percent = max_cpu_percent
        self.current_memory = 0
        self.current_cpu = 0.0
        self._lock = threading.Lock()

    def acquire(self, memory_mb: int, cpu_percent: float) -> bool:
        """Versucht Ressourcen zu reservieren"""
        with self._lock:
            if (self.current_memory + memory_mb <= self.max_memory_mb and
                self.current_cpu + cpu_percent <= self.max_cpu_percent):
                self.current_memory += memory_mb
                self.current_cpu += cpu_percent
                return True
            return False

    def release(self, memory_mb: int, cpu_percent: float):
        """Gibt Ressourcen frei"""
        with self._lock:
            self.current_memory = max(0, self.current_memory - memory_mb)
            self.current_cpu = max(0.0, self.current_cpu - cpu_percent)

class JobQueueManager:
    """Job Queue Manager mit Priority Scheduling und Sicherheitsfunktionen"""
    
    def __init__(self, max_workers: int = 4,
                 max_memory_mb: int = 1024,
                 max_cpu_percent: float = 90.0):
        self.job_queue = queue.PriorityQueue()
        self.resource_limiter = ResourceLimiter(max_memory_mb, max_cpu_percent)
        self.executor = ThreadPoolExecutor(max_workers=max_workers)
        self.jobs: Dict[str, Job] = {}
        self._lock = threading.Lock()
        self.logger = logging.getLogger(__name__)

    def submit_job(self, func: Callable, *args,
                  priority: JobPriority = JobPriority.MEDIUM,
                  timeout: int = 300,
                  max_retries: int = 3,
                  memory_mb: int = 100,
                  cpu_percent: float = 25.0,
                  **kwargs) -> str:
        """Fügt einen neuen Job zur Queue hinzu"""
        
        # Job ID generieren
        job_id = self._generate_job_id(func, args, kwargs)
        
        # Job erstellen
        job = Job(
            id=job_id,
            func=func,
            args=args,
            kwargs=kwargs,
            priority=priority,
            timeout=timeout,
            max_retries=max_retries
        )
        
        # Job sicher serialisieren
        try:
            SafeSerializer.serialize((args, kwargs))
        except ValueError as e:
            raise ValueError(f"Job-Parameter nicht serialisierbar: {e}")
            
        # Job zur Queue hinzufügen
        with self._lock:
            self.jobs[job_id] = job
            self.job_queue.put((-priority.value, job_id))
            
        self.logger.info(f"Job {job_id} submitted with priority {priority}")
        return job_id

    def execute_job(self, job: Job) -> None:
        """Führt einen Job aus und behandelt Fehler und Timeouts"""
        
        if not self.resource_limiter.acquire(100, 25.0):
            self.logger.warning(f"Job {job.id}: Nicht genügend Ressourcen verfügbar")
            job.status = JobStatus.FAILED
            job.error = "Insufficient resources"
            return

        try:
            job.status = JobStatus.RUNNING
            
            # Job mit Timeout ausführen
            result = self._run_with_timeout(job)
            
            job.result = result
            job.status = JobStatus.COMPLETED
            self.logger.info(f"Job {job.id} completed successfully")
            
        except TimeoutError:
            job.status = JobStatus.TIMEOUT
            job.error = "Job timed out"
            self.logger.error(f"Job {job.id} timed out")
            
            # Retry wenn möglich
            if job.retry_count < job.max_retries:
                job.retry_count += 1
                self.job_queue.put((-job.priority.value, job.id))
                self.logger.info(f"Job {job.id} requeued for retry {job.retry_count}")
                
        except Exception as e:
            job.status = JobStatus.FAILED
            job.error = str(e)
            self.logger.error(f"Job {job.id} failed: {e}")
            
            # Retry wenn möglich
            if job.retry_count < job.max_retries:
                job.retry_count += 1
                self.job_queue.put((-job.priority.value, job.id))
                self.logger.info(f"Job {job.id} requeued for retry {job.retry_count}")
        
        finally:
            self.resource_limiter.release(100, 25.0)

    def _run_with_timeout(self, job: Job) -> Any:
        """Führt einen Job mit Timeout aus"""
        future = self.executor.submit(self._safe_execute, job)
        try:
            return future.result(timeout=job.timeout)
        except TimeoutError:
            future.cancel()
            raise

    def _safe_execute(self, job: Job) -> Any:
        """Führt einen Job sicher aus"""
        try:
            # Parameter deserialisieren
            args = SafeSerializer.deserialize(
                SafeSerializer.serialize(job.args)
            )
            kwargs = SafeSerializer.deserialize(
                SafeSerializer.serialize(job.kwargs)
            )
            
            # Job ausführen
            return job.func(*args, **kwargs)
            
        except Exception as e:
            self.logger.error(f"Error executing job {job.id}: {e}")
            raise

    def _generate_job_id(self, func: Callable, args: tuple, kwargs: dict) -> str:
        """Generiert eine eindeutige Job-ID"""
        data = f"{func.__name__}{str(args)}{str(kwargs)}{time.time()}"
        return hashlib.md5(data.encode()).hexdigest()

    def get_job_status(self, job_id: str) -> Dict:
        """Gibt den aktuellen Status eines Jobs zurück"""
        job = self.jobs.get(job_id)
        if not job:
            raise ValueError(f"Job {job_id} nicht gefunden")
            
        return {
            "id": job.id,
            "status": job.status.value,
            "retry_count": job.retry_count,
            "result": job.result if job.status == JobStatus.COMPLETED else None,
            "error": job.error if job.status in (JobStatus.FAILED, JobStatus.TIMEOUT) else None
        }

    def run(self):
        """Startet die Job-Verarbeitung"""
        while True:
            try:
                # Nächsten Job aus Queue holen
                _, job_id = self.job_queue.get()
                job = self.jobs[job_id]
                
                # Job ausführen
                self.execute_job(job)
                
            except Exception as e:
                self.logger.error(f"Error in job queue processing: {e}")
                
            finally:
                self.job_queue.task_done()

# Beispiel-Verwendung:
if __name__ == "__main__":
    # Logging konfigurieren
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s'
    )
    
    # Queue Manager erstellen
    manager = JobQueueManager(
        max_workers=4,
        max_memory_mb=1024,
        max_cpu_percent=90.0
    )
    
    # Queue-Verarbeitung in separatem Thread starten
    threading.Thread(target=manager.run, daemon=True).start()
    
    # Beispiel-Job definieren
    def example_job(x: int, y: int) -> int:
        time.sleep(2)  # Simuliere Arbeit
        return x + y
    
    # Jobs einreichen
    job_id = manager.submit_job(
        example_job,
        10, 20,
        priority=JobPriority.HIGH,
        timeout=5
    )
    
    # Warte und prüfe Status
    time.sleep(3)
    status = manager.get_job_status(job_id)
    print(f"Job Status: {status}")
