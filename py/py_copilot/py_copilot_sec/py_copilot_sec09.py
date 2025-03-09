import threading
from collections import OrderedDict

class LRUCacheManager:
    def __init__(self, max_size, backend_store):
        self.max_size = max_size
        self.backend_store = backend_store
        self.cache = OrderedDict()
        self.lock = threading.Lock()

    def get(self, key):
        with self.lock:
            if key in self.cache:
                value = self.cache[key]
                self.cache.move_to_end(key)
                return value
            else:
                value = self.backend_store.get(key)
                if value is not None:
                    self.cache[key] = value
                    self.cache.move_to_end(key)
                return value

    def set(self, key, value):
        with self.lock:
            if key in self.cache:
                self.cache.move_to_end(key)
            self.cache[key] = value
            if len(self.cache) > self.max_size:
                self.cache.popitem(last=False)
            self.backend_store.set(key, value)

    def delete(self, key):
        with self.lock:
            if key in self.cache:
                del self.cache[key]
            self.backend_store.delete(key)

    def clear(self):
        with self.lock:
            self.cache.clear()
            self.backend_store.clear()
