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
        """Generiert einen eindeutigen Cache-Key basierend auf Funktion und Parametern"""
        # Wir hashen die Parameterwerte um sensible Daten zu schützen
        key_parts = [func.__name__]
        
        for arg in args:
            key_parts.append(str(hash(str(arg))))
        
        for k, v in sorted(kwargs.items()):
            key_parts.append(f"{k}:{hash(str(v))}")
            
        key = ":".join(key_parts)
        return hashlib.sha256(key.encode()).hexdigest()

    def get(self, key: str) -> Optional[Any]:
        """Thread-safe Zugriff auf Cache-Einträge"""
        with self._lock:
            if key not in self._cache:
                return None
            
            entry = self._cache[key]
            
            # Prüfe ob der Eintrag abgelaufen ist
            if time.time() - entry.timestamp > self._ttl:
                del self._cache[key]
                return None
                
            return entry.value

    def set(self, key: str, value: Any, sensitive: bool = False) -> None:
        """Thread-safe Setzen von Cache-Einträgen"""
        with self._lock:
            # Sensitive Daten werden nicht gecached
            if sensitive:
                logger.warning(f"Skipping cache for sensitive data with key {key}")
                return
                
            self._cache[key] = CacheEntry(
                value=value,
                timestamp=time.time(),
                sensitive=sensitive
            )

    def invalidate(self, key: str) -> None:
        """Invalidiert einen einzelnen Cache-Eintrag"""
        with self._lock:
            if key in self._cache:
                del self._cache[key]

    def clear(self) -> None:
        """Leert den gesamten Cache"""
        with self._lock:
            self._cache.clear()

def cached(ttl: int = 300, sensitive_params: tuple = ()):
    """
    Decorator für Method-Caching mit:
    - Cache-Invalidierung durch TTL
    - Thread-Safety
    - Schutz sensitiver Daten
    - Vermeidung von Race Conditions
    
    Args:
        ttl: Time-to-live in Sekunden
        sensitive_params: Namen der Parameter die sensitive Daten enthalten
    """
    cache = SecureCache(ttl=ttl)
    
    def decorator(func: Callable) -> Callable:
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            # Prüfe ob sensitive Parameter übergeben wurden
            has_sensitive = any(
                param in kwargs or 
                (i < len(args) and param == func.__code__.co_varnames[i])
                for i, param in enumerate(sensitive_params)
            )
            
            cache_key = cache._generate_key(func, args, kwargs)
            
            # Versuche zunächst aus dem Cache zu lesen
            if not has_sensitive:
                cached_value = cache.get(cache_key)
                if cached_value is not None:
                    logger.debug(f"Cache hit for {func.__name__}")
                    return cached_value
            
            # Bei Cache-Miss: Führe die Funktion aus
            result = func(*args, **kwargs)
            
            # Cache das Ergebnis für nicht-sensitive Daten
            cache.set(cache_key, result, sensitive=has_sensitive)
            
            return result
            
        # Expose Cache-Operationen
        wrapper.invalidate = lambda: cache.invalidate(
            cache._generate_key(func, args, kwargs)
        )
        wrapper.clear = cache.clear
        
        return wrapper
        
    return decorator

# Beispiel Verwendung:
@cached(ttl=60, sensitive_params=("password", "token"))
def get_user_data(user_id: int, password: str = None) -> dict:
    # Simuliere DB-Zugriff
    time.sleep(1)
    return {"id": user_id, "name": f"User {user_id}"}

# Thread-safety Demo
def worker(user_id: int):
    result = get_user_data(user_id)
    logger.info(f"Thread {threading.current_thread().name}: {result}")

if __name__ == "__main__":
    # Demo mit mehreren Threads
    threads = []
    for i in range(3):
        t = threading.Thread(target=worker, args=(42,))
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()
        
    # Cache invalidieren
    get_user_data.invalidate()
    
    # Mit sensitivem Parameter - wird nicht gecached
    result_sensitive = get_user_data(42, password="secret")