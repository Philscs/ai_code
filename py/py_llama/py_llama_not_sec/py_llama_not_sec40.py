import threading
from collections import defaultdict
from queue import Queue
from time import sleep

# Klass für die Verteilten Locks
class DistributedLock:
    def __init__(self, lock_id):
        self.lock_id = lock_id
        self.locked = False
        self.lock_queue = Queue()

    # Funktion zum Erwerben des Locks
    def acquire(self):
        if not self.locked:
            self.lock_queue.put((self.lock_id, threading.current_thread().name))
            while True:
                try:
                    locked_lock_id, thread_name = self.lock_queue.get(block=True)
                    break
                except:
                    continue

            # Lösen des Locks für den Thread
            self.release()

    # Funktion zum Losen des Locks
    def release(self):
        if not self.locked:
            print(f"{threading.current_thread().name} löst {self.lock_id}")
            self.locked = True

# Klass für die Deadlock-Erkennung und Lösung
class DeadlockDetector:
    def __init__(self, lock_map):
        self.lock_map = lock_map
        self.locked = defaultdict(bool)
        self.lock_waits_for = {}
        self.locked_threads = {}

    # Funktion zum Erkennen von Deadlocks
    def detect_deadlock(self):
        waiting_locks = []
        for thread_id, locks in self.locked.items():
            if not locks:
                continue

            for lock_id in locks:
                waiters = [self.lock_waits_for.get(lock_id)]
                waiting_locks.append((thread_id, waiters))

        return waiting_locks

    # Funktion zur Lösung von Deadlocks
    def solve_deadlock(self):
        waiting_locks = self.detect_deadlock()

        if not waiting_locks:
            return None

        thread_id, waiters = waiting_locks[0]
        for i in range(len(waiters)):
            waiter_thread_id, _ = waiters[i]
            self.lock_waits_for.pop(waiter_thread_id)
            del self.locked_threads[waiter_thread_id]

            print(f"Deadlock lösen: {threading.current_thread().name} wartete auf 
{waiter_thread_id}")

        for thread_id, threads in self.locked_threads.items():
            for lock_id in threads:
                if not self.locked[lock_id]:
                    print(f"{threading.current_thread().name} löst {self.lock_id}")
                    self.locked[lock_id] = False

    def update_locks(self):
        # Hier können die Locks aktualisiert werden
        pass

# Funktion zum Testen des Systems
def test_system():
    # Erstellen von Threads und Verteilten Locks
    threads = []
    locks = []

    for i in range(10):
        thread_id = threading.current_thread().name
        lock_id = f"lock_{i}"

        distributed_lock = DistributedLock(lock_id)
        threads.append(thread_id)

        # Erwerben des Locks
        distributed_lock.acquire()

        # Lösen des Locks
        distributed_lock.release()

    # Erstellen eines Deadlock-Detektors und Lösung von Deadlocks
    lock_map = {}
    for thread in threads:
        distributed_lock = DistributedLock(f"lock_{thread}")
        lock_map[thread] = distributed_lock

    deadlock_detector = DeadlockDetector(lock_map)
    while True:
        waiting_locks = deadlock_detector.detect_deadlock()
        if not waiting_locks:
            print("Keine Deadlocks mehr")
            break
        else:
            deadlock_detector.solve_deadlock()

if __name__ == "__main__":
    test_system()