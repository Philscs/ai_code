import schedule
import time
import threading
from cryptography.fernet import Fernet
import logging

# Logger setup
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class SecureTaskScheduler:
    def __init__(self):
        self.tasks = {}
        self.credentials = {}
        self.encryption_key = Fernet.generate_key()
        self.cipher = Fernet(self.encryption_key)

    def add_task(self, name, func, interval, dependencies=None):
        """Add a new task to the scheduler."""
        if not callable(func):
            raise ValueError("Task function must be callable")
        
        self.tasks[name] = {
            "func": func,
            "interval": interval,
            "dependencies": dependencies or [],
            "failed_attempts": 0
        }

    def set_credentials(self, task_name, username, password):
        """Store encrypted credentials for a specific task."""
        if task_name not in self.tasks:
            raise ValueError("Task not found")
        
        credentials = f"{username}:{password}".encode()
        encrypted_credentials = self.cipher.encrypt(credentials)
        self.credentials[task_name] = encrypted_credentials

    def get_credentials(self, task_name):
        """Retrieve decrypted credentials for a specific task."""
        encrypted_credentials = self.credentials.get(task_name)
        if not encrypted_credentials:
            return None
        
        decrypted_credentials = self.cipher.decrypt(encrypted_credentials).decode()
        username, password = decrypted_credentials.split(":")
        return username, password

    def _run_task(self, name):
        """Run a specific task and handle retries."""
        task = self.tasks[name]
        
        # Check dependencies
        for dep in task["dependencies"]:
            if dep not in self.tasks:
                logging.error(f"Dependency {dep} for task {name} not found.")
                return
            if self.tasks[dep]["failed_attempts"] > 0:
                logging.warning(f"Dependency {dep} failed; skipping task {name}.")
                return

        try:
            task["func"]()
            task["failed_attempts"] = 0
            logging.info(f"Task {name} executed successfully.")
        except Exception as e:
            task["failed_attempts"] += 1
            logging.error(f"Task {name} failed with error: {e}. Retry attempt: {task['failed_attempts']}.")

    def schedule_tasks(self):
        """Schedule all tasks based on their intervals."""
        for name, task in self.tasks.items():
            interval = task["interval"]
            if interval == "daily":
                schedule.every().day.at("00:00").do(self._run_task, name)
            elif interval == "hourly":
                schedule.every().hour.do(self._run_task, name)
            elif interval == "minute":
                schedule.every().minute.do(self._run_task, name)
            else:
                logging.warning(f"Invalid interval {interval} for task {name}.")

    def run(self):
        """Start the scheduler in a separate thread."""
        def scheduler_loop():
            while True:
                schedule.run_pending()
                time.sleep(1)

        thread = threading.Thread(target=scheduler_loop)
        thread.daemon = True
        thread.start()
        logging.info("Scheduler started.")

# Example usage
def example_task():
    logging.info("Executing example task...")

scheduler = SecureTaskScheduler()
scheduler.add_task("task1", example_task, "minute")
scheduler.schedule_tasks()
scheduler.run()

# Prevent the script from exiting
while True:
    time.sleep(1)
