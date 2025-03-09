import threading
import time
from collections import OrderedDict

class CacheNode:
    def __init__(self, capacity, eviction_strategy="LRU"):
        self.capacity = capacity
        self.eviction_strategy = eviction_strategy
        self.cache = OrderedDict()
        self.lock = threading.Lock()

    def get(self, key):
        with self.lock:
            if key in self.cache:
                if self.eviction_strategy == "LRU":
                    self.cache.move_to_end(key)
                return self.cache[key]
            return None

    def put(self, key, value):
        with self.lock:
            if key in self.cache:
                self.cache[key] = value
                if self.eviction_strategy == "LRU":
                    self.cache.move_to_end(key)
            else:
                if len(self.cache) >= self.capacity:
                    if self.eviction_strategy in ("LRU", "FIFO"):
                        self.cache.popitem(last=self.eviction_strategy == "LRU")
                self.cache[key] = value

class DistributedCache:
    def __init__(self, node_count=3, node_capacity=5, eviction_strategy="LRU", consistency="write-through"):
        self.nodes = [CacheNode(node_capacity, eviction_strategy) for _ in range(node_count)]
        self.consistency = consistency
        self.monitor_data = {
            "hits": 0,
            "misses": 0,
            "writes": 0,
        }

    def _get_node(self, key):
        return self.nodes[hash(key) % len(self.nodes)]

    def get(self, key):
        node = self._get_node(key)
        value = node.get(key)
        if value is not None:
            self.monitor_data["hits"] += 1
        else:
            self.monitor_data["misses"] += 1
        return value

    def put(self, key, value):
        node = self._get_node(key)
        if self.consistency == "write-through":
            node.put(key, value)
            self.monitor_data["writes"] += 1
        elif self.consistency == "write-behind":
            threading.Thread(target=self._delayed_write, args=(node, key, value)).start()

    def _delayed_write(self, node, key, value):
        time.sleep(1)
        node.put(key, value)
        self.monitor_data["writes"] += 1

    def monitor(self):
        return self.monitor_data

# Beispielnutzung:
cache = DistributedCache(node_count=3, node_capacity=2, eviction_strategy="LRU", consistency="write-through")

cache.put("a", 1)
cache.put("b", 2)
cache.put("c", 3)  # Sollte "a" verdrängen (LRU)

print(cache.get("a"))  # None, da "a" verdrängt wurde
print(cache.get("b"))  # 2
print(cache.get("c"))  # 3

# Monitoring-Daten
print(cache.monitor())
