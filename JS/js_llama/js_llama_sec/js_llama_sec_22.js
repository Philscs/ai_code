class ReactiveCache {
    constructor(maxSize = 100, ttl = 60 * 60) {
      this.cache = new Map();
      this.maxSize = maxSize;
      this.ttl = ttl;
    }
  
    get(key) {
      const cachedValue = this.cache.get(key);
      if (cachedValue !== undefined) {
        return cachedValue.value;
      } else {
        return undefined;
      }
    }
  
    set(key, value) {
      this.cache.set(key, { value, timestamp: Date.now() });
    }
  
    invalidate(key) {
      this.cache.delete(key);
    }
  
    warmup(key) {
      const cachedValue = this.get(key);
      if (cachedValue !== undefined) {
        return cachedValue;
      } else {
        throw new Error(`Key ${key} nicht in Cache`);
      }
    }
  
    isInvalidated(key) {
      const cachedValue = this.get(key);
      return cachedValue === undefined;
    }
  
    getMemoryUsage() {
      return Object.keys(this.cache).length * 4; // 4 Bytes pro Eintrag
    }
  
    setSecurityHeaders(response, headers = {}) {
      Object.assign(headers, {
        'Cache-Control': 'max-age=0, public',
        'Pragma': 'no-cache',
        'Expires': '-1',
      });
      response.headers = headers;
      return response;
    }
  
    preventDOS(request, headers = {}) {
      if (request.method === 'GET' && request.url.includes('?')) {
        const now = new Date().getTime();
        const timestamp = parseInt(request.url.split('?')[1].split('=')[1], 10);
        if (now - timestamp < 3600) {
          return false;
        }
      } else {
        headers['X-Rate-Limit'] = 'blocked';
        headers['Retry-After'] = '3600';
      }
      request.headers = headers;
      return request;
    }
  
    async fetch(url, options = {}) {
      try {
        const response = await fetch(url, options);
        response = this.setSecurityHeaders(response);
        return response;
      } catch (error) {
        console.error(error);
      }
    }
  
    async query(queryString) {
      try {
        const url = `https://example.com${queryString}`;
        const response = await this.fetch(url);
        return response.json();
      } catch (error) {
        console.error(error);
      }
    }
  }
  
  // Usage
  const cache = new ReactiveCache();
  
  async function main() {
    try {
      // Cache-Invalidation
      await cache.invalidate('test');
  
      // Cache-Warmup
      const result1 = await cache.warmup('test');
      if (result1 === undefined) {
        console.log('Key nicht in Cache');
      } else {
        console.log(result1);
      }
  
      // Memory-Management
      console.log(`Memory usage: ${cache.getMemoryUsage()} bytes`);
  
      // Security-Headers
      const response = await cache.fetch('https://example.com', { method: 'GET' });
      console.log(response.headers);
  
      // DOS-Prävention
      const request = {
        method: 'GET',
        url: 'https://example.com?timestamp=1643723900',
      };
      await cache.preventDOS(request);
    } catch (error) {
      console.error(error);
    }
  }
  
  main();