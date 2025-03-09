import functools
import threading
from typing import Callable, Any

class LRUCache:
    def __init__(self, maxsize: int):
        self.maxsize = maxsize
        self.cache = {}
        self.lock = threading.Lock()

    def get(self, key: str) -> Any:
        with self.lock:
            return self.cache.get(key)

    def set(self, key: str, value: Any) -> None:
        with self.lock:
            if key in self.cache:
                del self.cache[key]
            elif len(self.cache) >= self.maxsize:
                del self.cache[list(self.cache.keys())[0]]
            self.cache[key] = value

class CacheInvalidator:
    def __init__(self, cache: LRUCache):
        self.cache = cache
        self.validated_keys = set()

    def invalidate(self, key: str) -> None:
        with self.cache.lock:
            if key in self.validated_keys:
                del self.cache[key]
                self.validated_keys.remove(key)

def cached(cache_size: int, invalidator: CacheInvalidator) -> Callable[[Callable], Callable]:
    cache = LRUCache(cache_size)
    validated_keys = set()

    def decorator(func: Callable) -> Callable:
        @functools.wraps(func)
        def wrapper(*args: Any, **kwargs: Any) -> Any:
            result_key = (tuple(args), frozenset(kwargs.items()))
            if result_key in cache.cache:
                return cache.get(result_key)
            else:
                result = func(*args, **kwargs)
                if result not in validated_keys:
                    invalidated_keys.add((result_key,))
                validated_keys.add((result_key,))
                cache.set(result_key, result)
                return result
        return wrapper
    return decorator

def invalidate_cache(func: Callable) -> Callable:
    @functools.wraps(func)
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        result = func(*args, **kwargs)
        if result is not None and result in LRUCache().cache:
            LRUCache().invalidate((tuple(args), frozenset(kwargs.items())))
        return result
    return wrapper

# Beispiel für die Verwendung des Decorators:

class MyService:
    def calculate(self, x: int, y: int) -> float:
        # Hier wird eine sensible Operation ausgeführt.
        pass

my_service = MyService()

@cached(100)
def get_cached_result(x: int, y: int):
    # Diese Methode verwendet den cached Decorator.
    pass

# Beispiel für die Verwendung der invalidate_cache Funktion:

@invalidate_cache
def update_database():
    # Hier wird eine sensible Operation ausgeführt.
    pass
