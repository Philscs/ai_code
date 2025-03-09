import threading
import queue
import time
from concurrent.futures import ThreadPoolExecutor

class WorkStealingThreadPoolManager:
    def __init__(self, max_workers):
        self.max_workers = max_workers
        self.task_queues = [queue.Queue() for _ in range(max_workers)]
        self.lock = threading.Lock()
        self.threads = []
        self.shutdown_event = threading.Event()

        for i in range(max_workers):
            thread = threading.Thread(target=self.worker_loop, args=(i,))
            thread.daemon = True  # Daemon threads to ensure no thread leaks
            self.threads.append(thread)
            thread.start()

    def worker_loop(self, worker_id):
        local_queue = self.task_queues[worker_id]

        while not self.shutdown_event.is_set():
            try:
                # Try to get a task from the local queue
                task = local_queue.get(timeout=0.1)
                task()
                local_queue.task_done()
            except queue.Empty:
                # Work stealing: Try to steal from other workers
                for i in range(self.max_workers):
                    if i == worker_id:
                        continue

                    try:
                        task = self.task_queues[i].get_nowait()
                        task()
                        self.task_queues[i].task_done()
                        break
                    except queue.Empty:
                        continue

    def submit(self, func, *args, **kwargs):
        # Select the least loaded queue for task submission
        with self.lock:
            min_queue = min(self.task_queues, key=lambda q: q.qsize())
            min_queue.put(lambda: func(*args, **kwargs))

    def shutdown(self, wait=True):
        self.shutdown_event.set()
        if wait:
            for thread in self.threads:
                thread.join()

# Example usage
def example_task(task_id):
    print(f"Task {task_id} is running on thread {threading.current_thread().name}")
    time.sleep(1)

if __name__ == "__main__":
    pool = WorkStealingThreadPoolManager(max_workers=4)

    for i in range(10):
        pool.submit(example_task, i)

    pool.shutdown()
