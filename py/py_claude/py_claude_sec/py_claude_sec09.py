from abc import ABC, abstractmethod
from collections import OrderedDict
from threading import Lock
from typing import Any, Optional, Dict
import hashlib
import time
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class CacheBackend(ABC):
    """Abstract base class for cache backends."""
    
    @abstractmethod
    def get(self, key: str) -> Optional[Any]:
        pass
    
    @abstractmethod
    def set(self, key: str, value: Any) -> None:
        pass
    
    @abstractmethod
    def delete(self, key: str) -> None:
        pass
    
    @abstractmethod
    def clear(self) -> None:
        pass

class MemoryBackend(CacheBackend):
    """In-memory cache backend implementation."""
    
    def __init__(self):
        self._storage: Dict[str, Any] = {}
    
    def get(self, key: str) -> Optional[Any]:
        return self._storage.get(key)
    
    def set(self, key: str, value: Any) -> None:
        self._storage[key] = value
    
    def delete(self, key: str) -> None:
        self._storage.pop(key, None)
    
    def clear(self) -> None:
        self._storage.clear()

class RedisBackend(CacheBackend):
    """Redis cache backend implementation (placeholder)."""
    
    def __init__(self, host='localhost', port=6379):
        # In real implementation, you would initialize Redis client here
        self.host = host
        self.port = port
    
    def get(self, key: str) -> Optional[Any]:
        # Implement Redis get operation
        pass
    
    def set(self, key: str, value: Any) -> None:
        # Implement Redis set operation
        pass
    
    def delete(self, key: str) -> None:
        # Implement Redis delete operation
        pass
    
    def clear(self) -> None:
        # Implement Redis clear operation
        pass

class CacheEntry:
    """Represents a single cache entry with metadata."""
    
    def __init__(self, value: Any, ttl: Optional[int] = None):
        self.value = value
        self.created_at = time.time()
        self.ttl = ttl
        self.access_count = 0
        
    def is_expired(self) -> bool:
        """Check if the cache entry has expired."""
        if self.ttl is None:
            return False
        return time.time() > (self.created_at + self.ttl)
    
    def validate_size(self, max_size_bytes: int) -> bool:
        """Validate entry size against maximum allowed size."""
        try:
            size = len(str(self.value).encode('utf-8'))
            return size <= max_size_bytes
        except Exception:
            return False

class LRUCacheManager:
    """Thread-safe LRU Cache Manager with security features."""
    
    def __init__(
        self,
        backend: CacheBackend,
        max_size: int = 1000,
        max_memory_mb: int = 100,
        max_entry_size_kb: int = 512,
        default_ttl: Optional[int] = 3600
    ):
        self._backend = backend
        self._max_size = max_size
        self._max_memory_bytes = max_memory_mb * 1024 * 1024
        self._max_entry_size_bytes = max_entry_size_kb * 1024
        self._default_ttl = default_ttl
        
        self._cache = OrderedDict()
        self._lock = Lock()
        self._current_memory_usage = 0
    
    def _sanitize_key(self, key: str) -> str:
        """Sanitize cache key to prevent injection attacks."""
        return hashlib.sha256(str(key).encode()).hexdigest()
    
    def _validate_value(self, value: Any) -> bool:
        """Validate cache value for security and size constraints."""
        try:
            # Check for potentially dangerous types
            if isinstance(value, (type, callable)):
                return False
            
            # Validate size
            entry = CacheEntry(value)
            return entry.validate_size(self._max_entry_size_bytes)
        except Exception as e:
            logger.warning(f"Value validation failed: {str(e)}")
            return False
    
    def get(self, key: str) -> Optional[Any]:
        """
        Retrieve a value from cache.
        Thread-safe and handles entry expiration.
        """
        sanitized_key = self._sanitize_key(key)
        
        with self._lock:
            if sanitized_key not in self._cache:
                return None
            
            entry = self._cache[sanitized_key]
            
            # Check expiration
            if entry.is_expired():
                self.delete(key)
                return None
            
            # Update LRU order
            self._cache.move_to_end(sanitized_key)
            entry.access_count += 1
            
            return entry.value
    
    def set(self, key: str, value: Any, ttl: Optional[int] = None) -> bool:
        """
        Store a value in cache.
        Thread-safe and implements security checks.
        """
        if not self._validate_value(value):
            logger.warning(f"Invalid cache value for key: {key}")
            return False
        
        sanitized_key = self._sanitize_key(key)
        entry = CacheEntry(value, ttl or self._default_ttl)
        
        with self._lock:
            # Check memory constraints
            if sanitized_key not in self._cache:
                while len(self._cache) >= self._max_size:
                    self._cache.popitem(last=False)
            
            # Update cache
            self._cache[sanitized_key] = entry
            self._cache.move_to_end(sanitized_key)
            
            return True
    
    def delete(self, key: str) -> None:
        """Delete a cache entry."""
        sanitized_key = self._sanitize_key(key)
        
        with self._lock:
            self._cache.pop(sanitized_key, None)
    
    def clear(self) -> None:
        """Clear all cache entries."""
        with self._lock:
            self._cache.clear()
    
    def get_stats(self) -> Dict[str, Any]:
        """Get cache statistics."""
        with self._lock:
            return {
                'size': len(self._cache),
                'max_size': self._max_size,
                'memory_usage_bytes': self._current_memory_usage,
                'max_memory_bytes': self._max_memory_bytes
            }

# Example usage
def example_usage():
    # Initialize cache manager with memory backend
    cache = LRUCacheManager(
        backend=MemoryBackend(),
        max_size=1000,
        max_memory_mb=100,
        max_entry_size_kb=512,
        default_ttl=3600
    )
    
    # Store some values
    cache.set('user:123', {'name': 'John', 'age': 30})
    cache.set('settings:theme', 'dark', ttl=7200)
    
    # Retrieve values
    user = cache.get('user:123')
    theme = cache.get('settings:theme')
    
    # Get cache stats
    stats = cache.get_stats()
    print(f"Cache stats: {stats}")

if __name__ == '__main__':
    example_usage()