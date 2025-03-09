import threading
from collections import OrderedDict
from typing import Any, Callable, Optional

class CachePoisoningError(Exception):
    """Exception raised to indicate cache poisoning attempts."""
    pass

class LRUCache:
    def __init__(self, max_size: int, backend: Optional[Callable[[str], Any]] = None):
        """
        Initialize the LRU cache.

        Args:
            max_size (int): Maximum number of items in the cache.
            backend (Callable[[str], Any], optional): Function to fetch data if not in cache.
        """
        if max_size <= 0:
            raise ValueError("max_size must be greater than 0")
        
        self.max_size = max_size
        self.backend = backend
        self.cache = OrderedDict()
        self.lock = threading.RLock()

    def _validate_key_value(self, key: str, value: Any):
        """
        Validate the key and value to avoid cache poisoning.

        Args:
            key (str): The key to validate.
            value (Any): The value to validate.
        """
        if not isinstance(key, str):
            raise CachePoisoningError("Keys must be strings.")
        if value is None:
            raise CachePoisoningError("Values cannot be None.")

    def get(self, key: str) -> Any:
        """
        Retrieve a value from the cache or fetch it using the backend if not present.

        Args:
            key (str): The key to retrieve.

        Returns:
            Any: The value associated with the key.
        """
        with self.lock:
            if key in self.cache:
                # Move the accessed item to the end (most recently used)
                self.cache.move_to_end(key)
                return self.cache[key]

            if self.backend:
                value = self.backend(key)
                self.set(key, value)
                return value

            raise KeyError(f"Key {key} not found in cache and no backend available.")

    def set(self, key: str, value: Any):
        """
        Insert a key-value pair into the cache.

        Args:
            key (str): The key to insert.
            value (Any): The value to insert.
        """
        with self.lock:
            self._validate_key_value(key, value)

            if key in self.cache:
                self.cache.move_to_end(key)
            self.cache[key] = value

            if len(self.cache) > self.max_size:
                self.cache.popitem(last=False)  # Remove the least recently used item

    def delete(self, key: str):
        """
        Remove a key from the cache.

        Args:
            key (str): The key to remove.
        """
        with self.lock:
            if key in self.cache:
                del self.cache[key]

    def clear(self):
        """Clear all items from the cache."""
        with self.lock:
            self.cache.clear()

    def size(self) -> int:
        """Get the current size of the cache."""
        with self.lock:
            return len(self.cache)

    def __contains__(self, key: str) -> bool:
        """Check if a key exists in the cache."""
        with self.lock:
            return key in self.cache

# Example backend function
def backend_fetcher(key: str) -> str:
    # Simulate a fetch from a data source
    return f"Value_for_{key}"

# Example usage
if __name__ == "__main__":
    cache = LRUCache(max_size=3, backend=backend_fetcher)

    cache.set("a", "1")
    cache.set("b", "2")
    cache.set("c", "3")

    print(cache.get("a"))  # Output: "1"

    cache.set("d", "4")
    print("b" in cache)  # Output: False, "b" is evicted

    print(cache.get("e"))  # Fetches from backend
