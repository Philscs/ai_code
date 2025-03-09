import threading

class LockManager:
    def __init__(self):
        self.locks = {}
        self.waiting = {}

    def acquire(self, lock_id, thread_id):
        if lock_id not in self.locks:
            self.locks[lock_id] = threading.Lock()

        lock = self.locks[lock_id]
        if lock.acquire(blocking=False):
            return True

        if lock_id not in self.waiting:
            self.waiting[lock_id] = []

        self.waiting[lock_id].append(thread_id)
        return False

    def release(self, lock_id, thread_id):
        if lock_id not in self.locks:
            return

        lock = self.locks[lock_id]
        lock.release()

        if lock_id in self.waiting and self.waiting[lock_id]:
            next_thread = self.waiting[lock_id].pop(0)
            threading.Thread(target=self.acquire, args=(lock_id, next_thread)).start()

    def detect_deadlocks(self):
        for lock_id, waiting_threads in self.waiting.items():
            for thread_id in waiting_threads:
                if self._is_deadlocked(lock_id, thread_id, set()):
                    return True
        return False

    def _is_deadlocked(self, lock_id, thread_id, visited):
        if thread_id in visited:
            return True

        visited.add(thread_id)
        if lock_id in self.waiting:
            for next_thread_id in self.waiting[lock_id]:
                if self._is_deadlocked(lock_id, next_thread_id, visited):
                    return True

        visited.remove(thread_id)
        return False
