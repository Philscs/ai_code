import threading
from collections import OrderedDict
import logging

class LRUCache:
    def __init__(self, maxsize=128):
        self.maxsize = maxsize
        self.cache = OrderedDict()
        self.lock = threading.Lock()

    def get(self, key):
        with self.lock:
            if key in self.cache:
                value = self.cache.pop(key)
                self.cache[key] = value  # move to end
                return value
        return None

    def set(self, key, value):
        with self.lock:
            if key in self.cache:
                self.cache.pop(key)
            elif len(self.cache) >= self.maxsize:
                self.cache.popitem(last=False)  # remove oldest item
            self.cache[key] = value

class CacheManager:
    def __init__(self, backend='memory', maxsize=128):
        self.backend = backend
        self.cache = LRUCache(maxsize=maxsize)
        self.backend_classes = {
            'memory': MemoryBackend,
            # Add more backend classes as needed
        }

    def set_backend(self, backend_class):
        if backend_class not in self.backend_classes:
            raise ValueError("Invalid backend class")
        self.backend = backend_class

class MemoryBackend:
    def __init__(self, cache_manager):
        self.cache_manager = cache_manager

    def get(self, key):
        return self.cache_manager.cache.get(key)

    def set(self, key, value):
        self.cache_manager.cache.set(key, value)

class DiskBackend:
    def __init__(self, cache_manager):
        self.cache_manager = cache_manager
        # Initialize disk backend (e.g., using a database or file system)
        pass

    def get(self, key):
        return self.cache_manager.cache.get(key)

    def set(self, key, value):
        self.cache_manager.cache.set(key, value)

class CacheManagerThreadSafe:
    def __init__(self, maxsize=128):
        self.maxsize = maxsize
        self.lock = threading.Lock()
        self.cache = OrderedDict()

    def get(self, key):
        with self.lock:
            if key in self.cache:
                value = self.cache.pop(key)
                self.cache[key] = value  # move to end
                return value
        return None

    def set(self, key, value):
        with self.lock:
            if key in self.cache:
                self.cache.pop(key)
            elif len(self.cache) >= self.maxsize:
                self.cache.popitem(last=False)  # remove oldest item
            self.cache[key] = value

# Usage example:
cache_manager_memory = CacheManager(backend='memory', maxsize=128)
cache_manager_disk = CacheManager(backend='disk', maxsize=128)

def set_value(key, value):
    cache_manager_memory.set(key, value)
    cache_manager_disk.set(key, value)

def get_value(key):
    return cache_manager_memory.get(key)

# Thread-safe implementation:
class CacheManagerThreadSafe:
    def __init__(self, maxsize=128):
        self.maxsize = maxsize
        self.lock = threading.Lock()
        self.cache = OrderedDict()

    def get(self, key):
        with self.lock:
            if key in self.cache:
                value = self.cache.pop(key)
                self.cache[key] = value  # move to end
                return value
        return None

    def set(self, key, value):
        with self.lock:
            if key in self.cache:
                self.cache.pop(key)
            elif len(self.cache) >= self.maxsize:
                self.cache.popitem(last=False)  # remove oldest item
            self.cache[key] = value

# Usage example:
cache_manager_thread_safe = CacheManagerThreadSafe(maxsize=128)

def set_value(key, value):
    cache_manager_thread_safe.set(key, value)

def get_value(key):
    return cache_manager_thread_safe.get(key)
