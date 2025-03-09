import functools
import threading
import time
import hashlib
import logging
from typing import Any, Callable, Dict, Optional, Tuple
from dataclasses import dataclass

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@dataclass
class CacheEntry:
    value: Any
    timestamp: float
    sensitive: bool = False

class SecureCache:
    def __init__(self, ttl: int = 300):
        self._cache: Dict[str, CacheEntry] = {}
        self._lock = threading.RLock()
        self._ttl = ttl
    
    def _generate_key(self, func: Callable, args: Tuple, kwargs: Dict) -> str:
        """Generates a unique cache key based on function and parameters"""
        key_parts = [func.__name__]
        
        for arg in args:
            key_parts.append(str(hash(str(arg))))
        
        for k, v in sorted(kwargs.items()):
            key_parts.append(f"{k}:{hash(str(v))}")
            
        key = ":".join(key_parts)
        return hashlib.sha256(key.encode()).hexdigest()

    def get(self, key: str) -> Optional[Any]:
        """Thread-safe access to cache entries"""
        with self._lock:
            if key not in self._cache:
                return None
            
            entry = self._cache[key]
            
            # Check if the entry has expired
            if time.time() - entry.timestamp > self._ttl:
                del self._cache[key]
                return None
                
            return entry.value

    def set(self, key: str, value: Any, sensitive: bool = False) -> None:
        """Thread-safe setting of cache entries"""
        with self._lock:
            # Do not cache sensitive data
            if sensitive:
                logger.warning(f"Skipping cache for sensitive data with key {key}")
                return
                
            self._cache[key] = CacheEntry(
                value=value,
                timestamp=time.time(),
                sensitive=sensitive
            )

    def invalidate(self, key: str) -> None:
        """Invalidates a single cache entry"""
        with self._lock:
            if key in self._cache:
                del self._cache[key]

    def clear(self) -> None:
        """Clears the entire cache"""
        with self._lock:
            self._cache.clear()

def cached(ttl: int = 300, sensitive_params: tuple = ()):
    """
    Decorator for method caching with:
    - Cache invalidation by TTL
    - Thread safety
    - Protection of sensitive data
    - Avoidance of race conditions
    
    Args:
        ttl: Time-to-live in seconds
        sensitive_params: Names of parameters that contain sensitive data
    """
    cache = SecureCache(ttl=ttl)
    
    def decorator(func: Callable) -> Callable:
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            # Check if sensitive parameters were passed
            has_sensitive = any(
                param in kwargs or 
                (i < len(args) and param == func.__code__.co_varnames[i])
                for i, param in enumerate(sensitive_params)
            )
            
            cache_key = cache._generate_key(func, args, kwargs)
            
            # Try to read from the cache first
            if not has_sensitive:
                cached_value = cache.get(cache_key)
                if cached_value is not None:
                    logger.debug(f"Cache hit for {func.__name__}")
                    return cached_value
            
            # Cache miss: execute the function
            result = func(*args, **kwargs)
            
            # Cache the result for non-sensitive data
            cache.set(cache_key, result, sensitive=has_sensitive)
            
            return result
            
        # Expose cache operations
        wrapper.invalidate = lambda: cache.invalidate(
            cache._generate_key(func, args, kwargs)
        )
        wrapper.clear = cache.clear
        
        return wrapper
        
    return decorator

# Example usage:
@cached(ttl=60, sensitive_params=("password", "token"))
def get_user_data(user_id: int, password: str = None) -> dict:
    # Simulate DB access
    time.sleep(1)
    return {"id": user_id, "name": f"User {user_id}"}

# Thread-safety demo
def worker(user_id: int):
    result = get_user_data(user_id)
    logger.info(f"Thread {threading.current_thread().name}: {result}")

if __name__ == "__main__":
    # Demo with multiple threads
    threads = []
    for i in range(3):
        t = threading.Thread(target=worker, args=(42,))
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()
        
    # Invalidate the cache
    get_user_data.invalidate()
    
    # With sensitive parameter - not cached
    result_sensitive = get_user_data(42, password="secret")
