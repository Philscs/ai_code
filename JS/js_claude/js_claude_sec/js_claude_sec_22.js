class ReactiveCache {
    constructor(options = {}) {
      this.cache = new Map();
      this.maxSize = options.maxSize || 1000; // Maximum number of items
      this.ttl = options.ttl || 3600000; // Default TTL: 1 hour
      this.warmupPaths = options.warmupPaths || [];
      this.securityHeaders = {
        'Cache-Control': 'no-store, max-age=0',
        'X-Content-Type-Options': 'nosniff',
        'X-Frame-Options': 'DENY',
        'X-XSS-Protection': '1; mode=block'
      };
      
      // DOS Prevention
      this.requestLimits = new Map();
      this.maxRequestsPerMinute = options.maxRequestsPerMinute || 100;
      
      // Start automatic cache cleanup
      this.startCleanupInterval();
      
      // Warmup cache if paths provided
      if (this.warmupPaths.length > 0) {
        this.warmupCache();
      }
    }
  
    // Cache key generation with sanitization
    generateKey(key) {
      return typeof key === 'string' ? 
        key.replace(/[^a-zA-Z0-9]/g, '_').toLowerCase() : 
        JSON.stringify(key);
    }
  
    // Get item from cache
    get(key) {
      const sanitizedKey = this.generateKey(key);
      
      if (this.isRateLimited(sanitizedKey)) {
        throw new Error('Rate limit exceeded');
      }
      
      const item = this.cache.get(sanitizedKey);
      
      if (!item) {
        return null;
      }
      
      if (this.isExpired(item)) {
        this.delete(sanitizedKey);
        return null;
      }
      
      this.updateRequestCount(sanitizedKey);
      return item.value;
    }
  
    // Set item in cache
    set(key, value, customTTL = null) {
      const sanitizedKey = this.generateKey(key);
      
      if (this.cache.size >= this.maxSize) {
        this.evictOldest();
      }
      
      const item = {
        value,
        timestamp: Date.now(),
        ttl: customTTL || this.ttl,
        accessCount: 0
      };
      
      this.cache.set(sanitizedKey, item);
      return true;
    }
  
    // Delete item from cache
    delete(key) {
      const sanitizedKey = this.generateKey(key);
      return this.cache.delete(sanitizedKey);
    }
  
    // Clear entire cache
    clear() {
      this.cache.clear();
      return true;
    }
  
    // Check if item is expired
    isExpired(item) {
      return Date.now() - item.timestamp > item.ttl;
    }
  
    // Evict oldest items when cache is full
    evictOldest() {
      let oldest = null;
      let oldestKey = null;
      
      for (const [key, item] of this.cache.entries()) {
        if (!oldest || item.timestamp < oldest.timestamp) {
          oldest = item;
          oldestKey = key;
        }
      }
      
      if (oldestKey) {
        this.delete(oldestKey);
      }
    }
  
    // Cache warmup functionality
    async warmupCache() {
      try {
        const warmupPromises = this.warmupPaths.map(async path => {
          try {
            const response = await fetch(path);
            const data = await response.json();
            this.set(path, data);
          } catch (error) {
            console.error(`Failed to warmup cache for path: ${path}`, error);
          }
        });
        
        await Promise.all(warmupPromises);
      } catch (error) {
        console.error('Cache warmup failed:', error);
      }
    }
  
    // Automatic cache cleanup
    startCleanupInterval() {
      setInterval(() => {
        for (const [key, item] of this.cache.entries()) {
          if (this.isExpired(item)) {
            this.delete(key);
          }
        }
      }, 60000); // Cleanup every minute
    }
  
    // DOS Prevention: Rate limiting
    isRateLimited(key) {
      const now = Date.now();
      const userRequests = this.requestLimits.get(key) || [];
      
      // Remove requests older than 1 minute
      const recentRequests = userRequests.filter(
        timestamp => now - timestamp < 60000
      );
      
      return recentRequests.length >= this.maxRequestsPerMinute;
    }
  
    // Update request count for rate limiting
    updateRequestCount(key) {
      const now = Date.now();
      const userRequests = this.requestLimits.get(key) || [];
      
      // Add current request timestamp
      userRequests.push(now);
      
      // Keep only requests from last minute
      const recentRequests = userRequests.filter(
        timestamp => now - timestamp < 60000
      );
      
      this.requestLimits.set(key, recentRequests);
    }
  
    // Get security headers
    getSecurityHeaders() {
      return { ...this.securityHeaders };
    }
  }
  
  // Usage example:
  const cache = new ReactiveCache({
    maxSize: 1000,
    ttl: 3600000, // 1 hour
    maxRequestsPerMinute: 100,
    warmupPaths: [
      '/api/frequently-accessed-data',
      '/api/common-settings'
    ]
  });
  
  // Example usage with security headers
  async function handleRequest(req, res) {
    try {
      const key = req.url;
      let data = cache.get(key);
      
      if (!data) {
        // Fetch data if not in cache
        data = await fetchDataFromSource(key);
        cache.set(key, data);
      }
      
      // Apply security headers
      const securityHeaders = cache.getSecurityHeaders();
      Object.entries(securityHeaders).forEach(([header, value]) => {
        res.setHeader(header, value);
      });
      
      return res.json(data);
    } catch (error) {
      if (error.message === 'Rate limit exceeded') {
        res.status(429).json({ error: 'Too many requests' });
      } else {
        res.status(500).json({ error: 'Internal server error' });
      }
    }
  }