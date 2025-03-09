class CacheManager {
    constructor({ ttl = 60000, maxSize = 100 } = {}) {
      this.ttl = ttl; // Time-to-live in milliseconds
      this.maxSize = maxSize; // Maximum number of items in the cache
      this.cache = new Map(); // Internal storage for the cache
    }
  
    // Set a value in the cache
    set(key, value) {
      const now = Date.now();
      // If the key exists, delete it to refresh its position in the LRU order
      if (this.cache.has(key)) {
        this.cache.delete(key);
      } else if (this.cache.size >= this.maxSize) {
        // Evict the least recently used item if the cache exceeds the max size
        const oldestKey = this.cache.keys().next().value;
        this.cache.delete(oldestKey);
      }
  
      // Add the new item to the cache
      this.cache.set(key, { value, expiresAt: now + this.ttl });
    }
  
    // Get a value from the cache
    get(key) {
      const now = Date.now();
      const item = this.cache.get(key);
  
      if (!item) {
        return undefined; // Key does not exist
      }
  
      if (item.expiresAt < now) {
        // Item has expired, remove it from the cache
        this.cache.delete(key);
        return undefined;
      }
  
      // Refresh the item's position in the LRU order
      this.cache.delete(key);
      this.cache.set(key, item);
  
      return item.value;
    }
  
    // Delete a specific key from the cache
    delete(key) {
      return this.cache.delete(key);
    }
  
    // Clear the entire cache
    clear() {
      this.cache.clear();
    }
  
    // Get the current size of the cache
    size() {
      return this.cache.size;
    }
  }
  
  // Example usage:
  const cache = new CacheManager({ ttl: 5000, maxSize: 3 });
  
  cache.set("key1", "value1");
  cache.set("key2", "value2");
  cache.set("key3", "value3");
  console.log(cache.get("key1")); // Outputs: "value1"
  
  // Adding another key will evict the least recently used ("key2")
  cache.set("key4", "value4");
  console.log(cache.get("key2")); // Outputs: undefined
  
  // Works in both Browser and Node.js environments
  