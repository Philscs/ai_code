from abc import ABC, abstractmethod
from collections import OrderedDict
from datetime import datetime
import threading
import time
from typing import Any, Dict, Optional
import logging

# Eviction Strategien
class EvictionStrategy(ABC):
    @abstractmethod
    def evict(self, cache: Dict) -> str:
        pass

class LRUEviction(EvictionStrategy):
    def evict(self, cache: OrderedDict) -> str:
        return cache.popitem(last=False)[0]

class LFUEviction(EvictionStrategy):
    def __init__(self):
        self.frequency = {}
    
    def evict(self, cache: Dict) -> str:
        min_freq = min(self.frequency.values())
        for key, freq in self.frequency.items():
            if freq == min_freq:
                del self.frequency[key]
                return key

# Konsistenzprotokolle
class ConsistencyProtocol(ABC):
    @abstractmethod
    def update(self, key: str, value: Any) -> None:
        pass

class WriteThrough(ConsistencyProtocol):
    def __init__(self, storage):
        self.storage = storage
    
    def update(self, key: str, value: Any) -> None:
        self.storage.write(key, value)

class WriteBack(ConsistencyProtocol):
    def __init__(self, storage):
        self.storage = storage
        self.dirty_bits = set()
        self._start_background_flush()
    
    def update(self, key: str, value: Any) -> None:
        self.dirty_bits.add(key)
    
    def _start_background_flush(self):
        def flush_periodic():
            while True:
                for key in self.dirty_bits.copy():
                    self.storage.write(key, cache.get(key))
                    self.dirty_bits.remove(key)
                time.sleep(5)
        
        thread = threading.Thread(target=flush_periodic, daemon=True)
        thread.start()

# Monitoring
class CacheMonitor:
    def __init__(self):
        self.hits = 0
        self.misses = 0
        self.start_time = datetime.now()
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger("CacheMonitor")
    
    def record_hit(self):
        self.hits += 1
    
    def record_miss(self):
        self.misses += 1
    
    def get_stats(self):
        duration = (datetime.now() - self.start_time).total_seconds()
        hit_rate = self.hits / (self.hits + self.misses) if (self.hits + self.misses) > 0 else 0
        stats = {
            "hits": self.hits,
            "misses": self.misses,
            "hit_rate": hit_rate,
            "uptime_seconds": duration
        }
        self.logger.info(f"Cache Stats: {stats}")
        return stats

# Hauptcache-Klasse
class DistributedCache:
    def __init__(
        self,
        max_size: int = 1000,
        eviction_strategy: EvictionStrategy = LRUEviction(),
        consistency_protocol: ConsistencyProtocol = None,
    ):
        self.max_size = max_size
        self.cache = OrderedDict()
        self.eviction_strategy = eviction_strategy
        self.consistency_protocol = consistency_protocol
        self.monitor = CacheMonitor()
        self.lock = threading.Lock()
    
    def get(self, key: str) -> Optional[Any]:
        with self.lock:
            if key in self.cache:
                self.monitor.record_hit()
                # Bei LRU: Element ans Ende der OrderedDict verschieben
                if isinstance(self.eviction_strategy, LRUEviction):
                    value = self.cache.pop(key)
                    self.cache[key] = value
                return self.cache[key]
            self.monitor.record_miss()
            return None
    
    def put(self, key: str, value: Any) -> None:
        with self.lock:
            if len(self.cache) >= self.max_size:
                evicted_key = self.eviction_strategy.evict(self.cache)
                del self.cache[evicted_key]
            
            self.cache[key] = value
            if self.consistency_protocol:
                self.consistency_protocol.update(key, value)
    
    def get_stats(self):
        return self.monitor.get_stats()

# Beispiel für die Verwendung
class DummyStorage:
    def write(self, key: str, value: Any):
        print(f"Writing {key}:{value} to persistent storage")

if __name__ == "__main__":
    # Cache mit LRU und Write-Through
    storage = DummyStorage()
    cache = DistributedCache(
        max_size=3,
        eviction_strategy=LRUEviction(),
        consistency_protocol=WriteThrough(storage)
    )
    
    # Beispieloperationen
    cache.put("key1", "value1")
    cache.put("key2", "value2")
    cache.put("key3", "value3")
    print(cache.get("key1"))  # Hit
    cache.put("key4", "value4")  # Eviction von key2
    print(cache.get("key2"))  # Miss
    
    # Statistiken ausgeben
    print(cache.get_stats())