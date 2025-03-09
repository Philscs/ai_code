import threading
import queue
import time
import logging
import contextlib
from typing import Any, Callable, List, Optional
from dataclasses import dataclass
from concurrent.futures import Future

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@dataclass
class WorkerStats:
    tasks_completed: int = 0
    last_active: float = 0.0
    errors_encountered: int = 0

class IsolatedWorker:
    def __init__(self, worker_id: int, task_queue: queue.Queue, 
                 result_queue: queue.Queue, steal_from: List[queue.Queue]):
        self.worker_id = worker_id
        self.task_queue = task_queue
        self.result_queue = result_queue
        self.steal_from = steal_from
        self.thread: Optional[threading.Thread] = None
        self.running = False
        self.stats = WorkerStats()
        self._local = threading.local()
        
    def start(self):
        if not self.running:
            self.running = True
            self.thread = threading.Thread(target=self._worker_loop)
            self.thread.daemon = True
            self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)
            self.thread = None
            
    def _worker_loop(self):
        while self.running:
            try:
                # Versuche Task aus eigener Queue zu holen
                try:
                    task, future = self.task_queue.get(timeout=0.1)
                except queue.Empty:
                    # Work Stealing: Versuche Tasks von anderen Queues zu stehlen
                    task = future = None
                    for other_queue in self.steal_from:
                        try:
                            task, future = other_queue.get_nowait()
                            logger.debug(f"Worker {self.worker_id} stole task")
                            break
                        except queue.Empty:
                            continue
                    
                    if not task:
                        continue

                # Task ausführen in isoliertem Kontext
                try:
                    self._local.context = {}  # Isolierter Kontext pro Thread
                    result = task(**self._local.context)
                    future.set_result(result)
                    self.stats.tasks_completed += 1
                except Exception as e:
                    future.set_exception(e)
                    self.stats.errors_encountered += 1
                    logger.error(f"Error in worker {self.worker_id}: {str(e)}")
                finally:
                    self.stats.last_active = time.time()
                    self._local.context = {}
                    self.task_queue.task_done()

            except Exception as e:
                logger.error(f"Critical error in worker {self.worker_id}: {str(e)}")
                time.sleep(1)  # Prevent tight error loops

class ThreadPoolManager:
    def __init__(self, num_workers: int = None, 
                 max_queue_size: int = 1000,
                 worker_timeout: float = 300.0):
        self.num_workers = num_workers or (threading.cpu_count() * 2)
        self.max_queue_size = max_queue_size
        self.worker_timeout = worker_timeout
        
        self.workers: List[IsolatedWorker] = []
        self.queues: List[queue.Queue] = []
        self.result_queue = queue.Queue()
        self.shutdown_event = threading.Event()
        
        self._init_workers()
        self._start_monitor()
        
    def _init_workers(self):
        for i in range(self.num_workers):
            task_queue = queue.Queue(maxsize=self.max_queue_size)
            self.queues.append(task_queue)
            
            # Jeder Worker kann von allen anderen Queues stehlen
            steal_from = [q for q in self.queues if q != task_queue]
            
            worker = IsolatedWorker(
                worker_id=i,
                task_queue=task_queue,
                result_queue=self.result_queue,
                steal_from=steal_from
            )
            self.workers.append(worker)
            worker.start()

    def _start_monitor(self):
        """Startet Monitor-Thread für Worker-Recycling"""
        self.monitor_thread = threading.Thread(target=self._monitor_loop)
        self.monitor_thread.daemon = True
        self.monitor_thread.start()

    def _monitor_loop(self):
        while not self.shutdown_event.is_set():
            current_time = time.time()
            
            for i, worker in enumerate(self.workers):
                # Prüfe auf inaktive Worker
                if (current_time - worker.stats.last_active > self.worker_timeout and 
                    worker.stats.tasks_completed > 0):
                    logger.info(f"Recycling worker {worker.worker_id}")
                    
                    # Ersetze Worker
                    worker.stop()
                    new_worker = IsolatedWorker(
                        worker_id=worker.worker_id,
                        task_queue=worker.task_queue,
                        result_queue=self.result_queue,
                        steal_from=[q for q in self.queues if q != worker.task_queue]
                    )
                    new_worker.start()
                    self.workers[i] = new_worker

            time.sleep(60)  # Prüfe einmal pro Minute

    def submit(self, func: Callable, *args, **kwargs) -> Future:
        """Submit task to thread pool"""
        future = Future()
        
        # Wähle Queue mit kürzester Länge
        shortest_queue = min(self.queues, key=lambda q: q.qsize())
        
        # Verpacke Funktion und Argumente
        task = lambda context: func(*args, **kwargs)
        
        shortest_queue.put((task, future))
        return future

    @contextlib.contextmanager
    def batch_submit(self):
        """Context Manager für Batch-Submits"""
        futures = []
        try:
            yield futures
        finally:
            for future in futures:
                future.result()  # Warte auf Completion

    def shutdown(self, wait: bool = True):
        """Shutdown thread pool"""
        self.shutdown_event.set()
        
        for worker in self.workers:
            worker.stop()
            
        if wait:
            for queue in self.queues:
                queue.join()

        self.monitor_thread.join(timeout=1.0)

# Beispiel Verwendung:
if __name__ == "__main__":
    def example_task(x: int) -> int:
        time.sleep(0.1)  # Simuliere Arbeit
        return x * x

    # Pool erstellen
    pool = ThreadPoolManager(num_workers=4)

    try:
        # Einzelne Tasks submitten
        future1 = pool.submit(example_task, 5)
        print(f"Result 1: {future1.result()}")

        # Batch processing
        with pool.batch_submit() as futures:
            for i in range(10):
                future = pool.submit(example_task, i)
                futures.append(future)

    finally:
        pool.shutdown()