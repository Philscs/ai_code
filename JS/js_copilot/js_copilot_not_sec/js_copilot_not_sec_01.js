class CacheManager {
  constructor(lifespan, sizeLimit) {
    this.cache = new Map();
    this.lifespan = lifespan;
    this.sizeLimit = sizeLimit;
  }

  get(key) {
    const entry = this.cache.get(key);
    if (entry && Date.now() - entry.timestamp < this.lifespan) {
      // Update the entry timestamp to reflect its recent usage
      entry.timestamp = Date.now();
      return entry.value;
    }
    return null;
  }

  set(key, value) {
    if (this.cache.size >= this.sizeLimit) {
      // Evict the least recently used entry
      const lruKey = this.getLeastRecentlyUsedKey();
      this.cache.delete(lruKey);
    }
    this.cache.set(key, { value, timestamp: Date.now() });
  }

  getLeastRecentlyUsedKey() {
    let lruKey = null;
    let lruTimestamp = Infinity;
    for (const [key, entry] of this.cache) {
      if (entry.timestamp < lruTimestamp) {
        lruKey = key;
        lruTimestamp = entry.timestamp;
      }
    }
    return lruKey;
  }
}

// Example usage
const cache = new CacheManager(60000, 100); // Cache entries expire after 1 minute, maximum 100 entries

// Store a response in the cache
const response = { data: 'example data' };
cache.set('https://api.example.com/data', response);

// Retrieve a response from the cache
const cachedResponse = cache.get('https://api.example.com/data');
console.log(cachedResponse);
