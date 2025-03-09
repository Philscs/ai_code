import queue
import threading
import time
import subprocess
import pickle
import os

class Job:
    def __init__(self, command, priority=1, timeout=30, resources=None):
        """
        :param command: Command to be executed (list of arguments)
        :param priority: Priority of the job (lower value = higher priority)
        :param timeout: Maximum execution time for the job
        :param resources: Dictionary specifying resource limits (e.g., {'cpu': 50, 'memory': 256})
        """
        self.command = command
        self.priority = priority
        self.timeout = timeout
        self.resources = resources or {}
        self.status = "pending"  # Can be 'pending', 'running', 'failed', 'completed'
        self.result = None

class SafeJobSerializer:
    @staticmethod
    def serialize(job):
        """Serialize a job safely using pickle."""
        return pickle.dumps(job)

    @staticmethod
    def deserialize(data):
        """Deserialize job data safely."""
        return pickle.loads(data)

def enforce_resources(job):
    """Enforce resource limits before executing the job."""
    if job.resources.get('cpu'):
        os.system(f'cpulimit -l {job.resources["cpu"]}')
    if job.resources.get('memory'):
        os.system(f'ulimit -v {job.resources["memory"]}')

class JobQueueManager:
    def __init__(self):
        self.queue = queue.PriorityQueue()
        self.lock = threading.Lock()

    def add_job(self, job):
        """Add a job to the queue."""
        with self.lock:
            self.queue.put((job.priority, time.time(), job))

    def execute_job(self, job):
        """Execute a single job with timeout and resource handling."""
        def target():
            enforce_resources(job)
            try:
                result = subprocess.run(job.command, timeout=job.timeout, capture_output=True, text=True, check=True)
                job.result = result.stdout
                job.status = "completed"
            except subprocess.TimeoutExpired:
                job.status = "failed"
                job.result = "Timeout expired"
            except subprocess.CalledProcessError as e:
                job.status = "failed"
                job.result = f"Error: {e.stderr}"
            except Exception as e:
                job.status = "failed"
                job.result = f"Unhandled Exception: {str(e)}"

        thread = threading.Thread(target=target)
        thread.start()
        thread.join(job.timeout)
        if thread.is_alive():
            job.status = "failed"
            job.result = "Timeout expired"
            thread.join()

    def worker(self):
        while True:
            _, _, job = self.queue.get()
            if job is None:
                break
            job.status = "running"
            self.execute_job(job)
            print(f"Job completed with status: {job.status}, result: {job.result}")

    def start_workers(self, num_workers=2):
        """Start worker threads."""
        for _ in range(num_workers):
            threading.Thread(target=self.worker, daemon=True).start()

if __name__ == "__main__":
    manager = JobQueueManager()

    # Example jobs
    job1 = Job(command=["echo", "Hello, World!"], priority=1)
    job2 = Job(command=["sleep", "5"], priority=2, timeout=3)
    job3 = Job(command=["echo", "Job 3"], priority=0)

    manager.add_job(job1)
    manager.add_job(job2)
    manager.add_job(job3)

    manager.start_workers()

    time.sleep(10)  # Let jobs complete
