import threading
from queue import Queue
from collections import defaultdict
from typing import Callable, Any

class ThreadPool:
    def __init__(self, num_workers: int):
        self.num_workers = num_workers
        self.qs = [Queue() for _ in range(num_workers)]
        self.threads = []
        self.workers = []

        # Initialisierung der Worker-Instanzen
        for i in range(num_workers):
            worker = Worker(i, self)
            self.workers.append(worker)

    def submit(self, func: Callable, *args: Any, **kwargs: Any) -> int:
        # Auffordern eines neuen Threads
        if len(self.qs) < self.num_workers:
            thread = threading.Thread(target=self._execute, args=(func, args, kwargs), 
daemon=True)
            thread.start()
            return len(threading.enumerate())

        # Wenn kein neuer Thread benötigt wird, steale den nächsten verfügbaren Worker
        worker = next((w for w in self.workers if not w.is_busy), None)

        if worker:
            worker.submit(func, args, kwargs)
            return len(worker.queue.qsize())
        else:
            raise Exception("Kein leeres Buffer mehr")

    def _execute(self, func: Callable, *args: Any, **kwargs: Any):
        while True:
            item = self.qs.pop()
            if not item:
                break
            try:
                func(*item)
            except Exception as e:
                print(f"Exception occurred: {e}")
            finally:
                self._check_worker_state()


    def _check_worker_state(self):
        # Überprüfung, ob ein Worker verbraucht ist (nicht mehr auf der Queue steht).
        for worker in self.workers[:]:
            if not worker.is_busy and not worker.queue.qsize():
                self.workers.remove(worker)
                # Erneute Initialization mit einem neuen Thread
                new_worker = Worker(len(self.workers), self)
                self.workers.append(new_worker)


class Worker:
    def __init__(self, index: int, pool):
        self.index = index
        self.busy = False
        self.queue = Queue()
        self.pool = pool

    def submit(self, func: Callable, *args: Any, **kwargs: Any):
        # Übergeben einer Aufgabe an einen Worker.
        if not self.is_busy:
            self.busy = True
            try:
                self.queue.put((func, args, kwargs))
                self._next()
            except Exception as e:
                print(f"Exception occurred: {e}")

    def _next(self):
        # Wenn ein Worker aufgefüllt ist, stele den nächsten verfügbaren Worker.
        if len(self.pool.workers) > 1:
            next_worker_index = (self.index + 1) % len(self.pool.workers)
            next_worker = self.pool.workers[next_worker_index]
            next_worker.submit(self.queue.get()[0], *self.queue.get()[1:])