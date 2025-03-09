class Logger {
    constructor(name) {
      this.name = name;
    }
  
    use(target, propertyKey, descriptor) {
      const originalMethod = descriptor.value;
  
      descriptor.value = function (...args) {
        console.log(`[${this.name}] ${propertyKey} ->`, args);
        return originalMethod.apply(this, args);
      };
  
      return descriptor;
    }
  }
  
  class RateLimiter {
    constructor(maxRequestsPerMinute) {
      this.maxRequestsPerMinute = maxRequestsPerMinute;
      this.requestTimestamps = {};
    }
  
    use(target, propertyKey, descriptor) {
      const originalMethod = descriptor.value;
  
      descriptor.value = function (...args) {
        const now = Date.now();
        if (!this.requestTimestamps[this.name]) {
          this.requestTimestamps[this.name] = [];
        }
        this.requestTimestamps[this.name].push(now);
  
        while (this.requestTimestamps[this.name].length >= 1 &&
               this.requestTimestamps[this.name][0] <= now - 60000) {
          this.requestTimestamps[this.name].shift();
        }
  
        if (this.requestTimestamps[this.name].length > this.maxRequestsPerMinute) {
          throw new Error(`Rate limit exceeded for ${this.name}`);
        }
  
        return originalMethod.apply(this, args);
      };
  
      return descriptor;
    }
  }
  
  class Cache {
    constructor(maxCacheSize = 1000) {
      this.cache = {};
      this.maxCacheSize = maxCacheSize;
    }
  
    use(target, propertyKey, descriptor) {
      const originalMethod = descriptor.value;
  
      descriptor.value = function (...args) {
        const cacheKey = JSON.stringify(args);
        if (this.cache[cacheKey]) {
          return this.cache[cacheKey];
        } else {
          const result = originalMethod.apply(this, args);
          this.cache[cacheKey] = result;
          if (Object.keys(this.cache).length > this.maxCacheSize) {
            delete this.cache[this.cache.length - 1];
          }
          return result;
        }
      };
  
      return descriptor;
    }
  }
  
  class InputValidator {
    constructor(validations) {
      this.validations = validations;
    }
  
    use(target, propertyKey, descriptor) {
      const originalMethod = descriptor.value;
  
      descriptor.value = function (...args) {
        for (const validation in this.validations) {
          if (!this.validations[validation](...args)) {
            throw new Error(`Invalid input: ${validation}`);
          }
        }
  
        return originalMethod.apply(this, args);
      };
  
      return descriptor;
    }
  }
  
  class ErrorBoundary {
    constructor(next) {
      this.next = next;
    }
  
    use(target, propertyKey, descriptor) {
      const originalMethod = descriptor.value;
  
      descriptor.value = function (...args) {
        try {
          return originalMethod.apply(this, args);
        } catch (error) {
          console.error(error);
          return this.next.apply(this, args);
        }
      };
  
      return descriptor;
    }
  }
  
  export { Logger, RateLimiter, Cache, InputValidator, ErrorBoundary };

  import { Logger, RateLimiter, Cache, InputValidator, ErrorBoundary } from './decorator.js';
  
  class MyService {
    @Logger('my-service')
    async myMethod(data) {
      return data;
    }
  
    @RateLimiter(10)
    async fastMethod() {
      return 'Hello World!';
    }
  
    @Cache()
    async cachedMethod(data) {
      console.log(`Cache hit for ${JSON.stringify(data)}`);
      return data;
    }
  
    @InputValidator({
      required: ['name', 'age'],
      emailRegex: /\w+@\w+\.\w+/,
    })
    async validateMethod(name, age, email) {
      return { name, age };
    }
  }
  
  class MyServiceWithErrorBoundary extends ErrorBoundary(MyService) {}
  
  const myService = new MyService();
  const myServiceWithErrorBoundary = new MyServiceWithErrorBoundary();
  
  myService.myMethod({ name: 'John Doe', age: 30 });
  // => [object Object]
  
  myService.fastMethod();
  // => Hello World!
  
  myService.cachedMethod({ name: 'Jane Doe' });
  // => Cache hit for '{"name":"Jane Doe","age": null}'
  
  myService.validateMethod('john.doe@example.com', 25, 'invalid email');
  // => Error
  
  const result = myServiceWithErrorBoundary.myMethod({ name: 'John Doe', age: 30 });
  console.log(result); // => [object Object]