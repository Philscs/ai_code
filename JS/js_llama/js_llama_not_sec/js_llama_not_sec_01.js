class CacheManager {
    constructor(options = {}) {
      this.cache = {};
      this.lifespan = options.lifespan || 60 * 1000; // Default: 1 Minute
      this.sizeLimit = options.sizeLimit || 10 * 1024 * 1024; // Default: 10 MB
      this.lruStrategy = options.lruStrategy || 'timeBased';
    }
  
    get(key) {
      const cacheValue = this.cache[key];
      if (cacheValue) {
        this.updateCache(key, cacheValue);
        return cacheValue;
      }
      return null;
    }
  
    set(key, value, lifespan = null) {
      if (lifespan !== null && lifespan < this.lifespan) {
        throw new Error(`Lifespan must be greater than or equal to ${this.lifespan}ms`);
      }
  
      const cacheValue = JSON.parse(JSON.stringify(value));
      this.cache[key] = cacheValue;
  
      // LRU-Verdrängung
      if (this.lruStrategy === 'timeBased') {
        this.updateLRU();
      } else if (this.lruStrategy === 'sizeBased') {
        this.checkSizeLimit();
      }
    }
  
    updateCache(key, value) {
      const cacheValue = this.cache[key];
      delete this.cache[key];
      this.cache[key] = value;
    }
  
    updateLRU() {
      const lruKeys = Object.keys(this.cache);
      if (lruKeys.length > 0) {
        const now = Date.now();
        const oldestKey = lruKeys[0];
        const oldestValue = this.cache[oldestKey];
        delete this.cache[oldestKey];
        this.cache[oldestKey] = JSON.parse(JSON.stringify(oldestValue));
      }
    }
  
    checkSizeLimit() {
      if (Object.keys(this.cache).length > this.sizeLimit) {
        const keysToRemove = Object.keys(this.cache)
          .filter((key) => Date.now() - this.cache[key].timestamp > this.lifespan);
        keysToRemove.forEach((key) => delete this.cache[key]);
      }
    }
  
    invalidate(key) {
      delete this.cache[key];
    }
  }
  
  export default CacheManager;
  