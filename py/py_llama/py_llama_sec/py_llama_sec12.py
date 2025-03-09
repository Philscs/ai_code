from datetime import datetime, timedelta
import schedule
import time
from pydantic import BaseModel
from passlib.context import CryptContext

# Klassendefinierung der Tasks
class Task(BaseModel):
    id: int
    name: str
    interval_seconds: int
    run_at: datetime
    next_run_time: datetime = None

class Credential(BaseModel):
    username: str
    password: str

# Datenbankverbindung
import sqlite3

def db_connection():
    return sqlite3.connect('tasks.db')

# Sicherheitsfunktionen
from passlib.context import CryptContext

pwd_context = CryptContext(schemes=['bcrypt'], default='bcrypt')
class TaskScheduler:
    def __init__(self):
        self.tasks = []
        self.credentials = {}

    # Funktion zum Hinzufügen einer neuen Aufgabe
    def add_task(self, task: Task, credential: Credential):
        task.id = len(self.tasks) + 1
        self.tasks.append(task)
        if not task.next_run_time:
            task.next_run_time = datetime.now() + timedelta(seconds=task.interval_seconds)

    # Funktion zum Löschen einer Aufgabe
    def delete_task(self, id: int):
        for task in self.tasks:
            if task.id == id:
                self.tasks.remove(task)
                return True
        return False

    # Funktion zur Ausführung der Aufgaben
    def run_tasks(self):
        now = datetime.now()
        for task in self.tasks:
            if now >= task.next_run_time and now < task.run_at:
                print(f" Aufgabe {task.name} wird ausgeführt")
                # Hier die Aufgabe ausführen, z.B. eine Shell-Command
                import subprocess
                subprocess.run([f'command', '-c', f'your_command_here'])
            elif now >= task.next_run_time and now < datetime.now():
                self.tasks[task.id - 1].next_run_time = now + timedelta(seconds=task.interval_seconds)

    # Funktion zur Speicherung der Aufgaben
    def save_tasks(self):
        conn = db_connection()
        cursor = conn.cursor()

        for task in self.tasks:
            cursor.execute('INSERT INTO tasks (id, name, interval_seconds) VALUES (?, ?, ?)',
                           (task.id, task.name, task.interval_seconds))
            cursor.execute("SELECT id FROM tasks WHERE id = ?", (task.id,))
            task_id = cursor.fetchone()[0]
            if not task.credentials:
                credentials = Credential(username=task.name, password='default')
            else:
                credentials = self.credentials[task.name]

            cursor.execute('INSERT INTO credentials (id_task, username, password) VALUES (?, ?, ?)',
                           (task_id, credentials.username, 
pwd_context.hash(credentials.password)))
            conn.commit()

        conn.close()

    # Funktion zum Hinzufügen einer neuen Credential
    def add_credential(self, task_name: str, credential: Credential):
        if not self.credentials[task_name]:
            self.credentials[task_name] = credential

# Task-Scheduler-Objekt erstellen
scheduler = TaskScheduler()

# Aufgabe hinzufügen und Credential übergeben
scheduler.add_task(Task(id=1, name='Task 1', interval_seconds=60), Credential(username='User1', 
password='P@ssw0rd'))
scheduler.add_task(Task(id=2, name='Task 2', interval_seconds=30), Credential(username='User2', 
password='Password123'))

# Aufgabe löschen
scheduler.delete_task(1)

while True:
    scheduler.run_tasks()
    time.sleep(60) # 1 Sekunde warten

