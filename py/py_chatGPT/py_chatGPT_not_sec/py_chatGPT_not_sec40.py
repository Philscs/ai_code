import threading
import time
from collections import defaultdict

class DistributedLockManager:
    def __init__(self):
        self.locks = {}  # {resource_id: thread_id}
        self.waiting_graph = defaultdict(set)  # {thread_id: {blocked_by_thread_ids}}
        self.lock = threading.Lock()

    def acquire_lock(self, resource_id, thread_id):
        with self.lock:
            if resource_id not in self.locks:
                # Resource is free, acquire it
                self.locks[resource_id] = thread_id
                print(f"Thread {thread_id} acquired lock on {resource_id}")
                return True
            else:
                # Resource is taken, add to waiting graph
                current_holder = self.locks[resource_id]
                self.waiting_graph[thread_id].add(current_holder)
                print(f"Thread {thread_id} is waiting for lock on {resource_id}")

        # Check for deadlocks
        if self.detect_deadlock():
            self.resolve_deadlock()
        return False

    def release_lock(self, resource_id, thread_id):
        with self.lock:
            if resource_id in self.locks and self.locks[resource_id] == thread_id:
                del self.locks[resource_id]
                print(f"Thread {thread_id} released lock on {resource_id}")

                # Remove thread from waiting graph
                self.waiting_graph.pop(thread_id, None)

                # Remove thread from all other waiting lists
                for waiting_threads in self.waiting_graph.values():
                    waiting_threads.discard(thread_id)

    def detect_deadlock(self):
        """Detects if there is a cycle in the waiting graph."""
        visited = set()
        stack = set()

        def visit(node):
            if node in stack:
                return True
            if node in visited:
                return False

            visited.add(node)
            stack.add(node)
            for neighbor in self.waiting_graph[node]:
                if visit(neighbor):
                    return True
            stack.remove(node)
            return False

        for node in self.waiting_graph:
            if visit(node):
                return True

        return False

    def resolve_deadlock(self):
        """Resolves a deadlock by forcefully releasing locks."""
        print("Deadlock detected! Resolving...")
        with self.lock:
            # Simple approach: choose a thread to terminate/release its locks
            for thread_id in self.waiting_graph:
                for resource_id, holder_id in list(self.locks.items()):
                    if holder_id == thread_id:
                        print(f"Forcefully releasing lock on {resource_id} held by Thread {thread_id}")
                        del self.locks[resource_id]

                # Clear thread from waiting graph
                self.waiting_graph.pop(thread_id, None)
                for waiting_threads in self.waiting_graph.values():
                    waiting_threads.discard(thread_id)
                break

# Example usage
def worker(lock_manager, resource_id, thread_id):
    while not lock_manager.acquire_lock(resource_id, thread_id):
        time.sleep(1)  # Retry after some time

    time.sleep(2)  # Simulate work
    lock_manager.release_lock(resource_id, thread_id)

lock_manager = DistributedLockManager()
threads = []

for i in range(3):
    t = threading.Thread(target=worker, args=(lock_manager, "resource", f"Thread-{i}"))
    threads.append(t)
    t.start()

for t in threads:
    t.join()
