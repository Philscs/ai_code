import collections
from typing import Dict, Any

class CacheNode:
    def __init__(self, name: str):
        self.name = name
        self.cache = {}
        self.size_limit = 100

class EvictionStrategy:
    def __init__(self, strategy_name: str, evicted_key_func, size_limit):
        self.strategy_name = strategy_name
        self.evicted_key_func = evicted_key_func
        self.size_limit = size_limit

class LRUEvictionStrategy(EvictionStrategy):
    def __init__(self, size_limit=100):
        super().__init__("LRU", None, size_limit)

    def eviction(self) -> str:
        lru_cache = collections.OrderedDict(self.cache)
        if len(lru_cache) > self.size_limit:
            evicted_key = self.evicted_key_func(lru_cache.keys())
            del lru_cache[evicted_key]
        return evicted_key

class FIFOShuffleEvictionStrategy(EvictionStrategy):
    def __init__(self, size_limit=100):
        super().__init__("FIFO Shuffle", None, size_limit)

    def eviction(self) -> str:
        keys = list(self.cache.keys())
        shuffle_keys = [key for key in keys]
        random.shuffle(shuffle_keys)
        evicted_key = self.evicted_key_func(shuffle_keys[0])
        del self.cache[evicted_key]
        return evicted_key

class RandomEvictionStrategy(EvictionStrategy):
    def __init__(self, size_limit=100):
        super().__init__("Random", None, size_limit)

    def eviction(self) -> str:
        keys = list(self.cache.keys())
        random_index = random.randint(0, len(keys)-1)
        evicted_key = self.evicted_key_func(keys[random_index])
        del self.cache[evicted_key]
        return evicted_key

class ConsistencyProtocol:
    def __init__(self):
        self.cache = {}

class MultiVersionConsistencyProtocol(ConsistencyProtocol):
    def __init__(self, size_limit=100):
        super().__init__()
        self.cache = collections.OrderedDict()

class LastWriterWinsConsistencyProtocol(ConsistencyProtocol):
    def __init__(self, size_limit=100):
        super().__init__()
        self.cache = collections.OrderedDict()

class DistributedCache:
    def __init__(self, node_name: str, eviction_strategy: EvictionStrategy, consistency_protocol: 
ConsistencyProtocol):
        self.node_name = node_name
        self.cache_node = CacheNode(node_name)
        self.eviction_strategy = eviction_strategy
        self.consistency_protocol = consistency_protocol

    def add(self, key: Any, value: Any) -> None:
        if key in self.consistency_protocol.cache:
            del self.consistency_protocol.cache[key]
        self.consistency_protocol.cache[key] = value
        self.cache_node.cache[key] = value

    def get(self, key: Any) -> Any:
        return self.consistency_protocol.cache.get(key)

    def delete(self, key: Any) -> None:
        if key in self.consistency_protocol.cache:
            del self.consistency_protocol.cache[key]
            self.cache_node.cache.pop(key, None)
            evicted_key = self.eviction_strategy(eviction_key_func=lambda x:x)
            print(f"Node {self.node_name} has evicted key: {evicted_key}")

def main():
    lru_eviction_strategy = LRUEvictionStrategy()
    consistency_protocol = MultiVersionConsistencyProtocol()
    distributed_cache = DistributedCache("node1", lru_eviction_strategy, consistency_protocol)

    keys_to_add = [1, 2, 3, 4, 5]
    values_to_add = ["value1", "value2", "value3", "value4", "value5"]

    for key, value in zip(keys_to_add, values_to_add):
        distributed_cache.add(key, value)
        print(f"Key: {key}, Value: {value}")

    keys_to_delete = [1, 2, 4]

    for key in keys_to_delete:
        distributed_cache.delete(key)

if __name__ == "__main__":
    main()