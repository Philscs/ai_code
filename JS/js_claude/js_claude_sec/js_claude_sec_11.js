// Cache storage for method results
const methodCache = new Map();

// Rate limit tracking
const rateLimitStore = new Map();

/**
 * Method Caching Decorator
 * Caches method results based on input parameters
 * @param {number} ttl - Time to live in milliseconds
 */
export function cache(ttl = 5000) {
  return function (target, propertyKey, descriptor) {
    const originalMethod = descriptor.value;

    descriptor.value = function (...args) {
      const cacheKey = `${propertyKey}_${JSON.stringify(args)}`;
      const cachedItem = methodCache.get(cacheKey);

      if (cachedItem && Date.now() - cachedItem.timestamp < ttl) {
        console.log(`Cache hit for method ${propertyKey}`);
        return cachedItem.value;
      }

      const result = originalMethod.apply(this, args);
      methodCache.set(cacheKey, {
        value: result,
        timestamp: Date.now()
      });

      return result;
    };

    return descriptor;
  };
}

/**
 * Rate Limiting Decorator
 * Limits the number of method calls within a time window
 * @param {number} limit - Maximum number of calls
 * @param {number} window - Time window in milliseconds
 */
export function rateLimit(limit = 5, window = 1000) {
  return function (target, propertyKey, descriptor) {
    const originalMethod = descriptor.value;

    descriptor.value = function (...args) {
      const key = `${propertyKey}_${this.constructor.name}`;
      const now = Date.now();
      const calls = rateLimitStore.get(key) || [];

      // Remove expired calls
      const validCalls = calls.filter(timestamp => now - timestamp < window);

      if (validCalls.length >= limit) {
        throw new Error(`Rate limit exceeded for method ${propertyKey}`);
      }

      validCalls.push(now);
      rateLimitStore.set(key, validCalls);

      return originalMethod.apply(this, args);
    };

    return descriptor;
  };
}

/**
 * Input Validation Decorator
 * Validates method parameters against schema
 * @param {Object} schema - Validation schema for parameters
 */
export function validate(schema) {
  return function (target, propertyKey, descriptor) {
    const originalMethod = descriptor.value;

    descriptor.value = function (...args) {
      // Basic schema validation
      Object.keys(schema).forEach((param, index) => {
        const value = args[index];
        const rules = schema[param];

        if (rules.required && (value === undefined || value === null)) {
          throw new Error(`Parameter ${param} is required`);
        }

        if (rules.type && typeof value !== rules.type) {
          throw new Error(`Parameter ${param} must be of type ${rules.type}`);
        }

        if (rules.min !== undefined && value < rules.min) {
          throw new Error(`Parameter ${param} must be >= ${rules.min}`);
        }

        if (rules.max !== undefined && value > rules.max) {
          throw new Error(`Parameter ${param} must be <= ${rules.max}`);
        }

        if (rules.pattern && !rules.pattern.test(value)) {
          throw new Error(`Parameter ${param} does not match required pattern`);
        }
      });

      return originalMethod.apply(this, args);
    };

    return descriptor;
  };
}

/**
 * Logging Decorator
 * Logs method calls, parameters, and execution time
 * @param {Object} options - Logging options
 */
export function log(options = { params: true, result: true, timing: true }) {
  return function (target, propertyKey, descriptor) {
    const originalMethod = descriptor.value;

    descriptor.value = function (...args) {
      const startTime = Date.now();
      const className = this.constructor.name;

      // Log method call and parameters
      if (options.params) {
        console.log(`[${className}.${propertyKey}] Called with:`, args);
      }

      try {
        const result = originalMethod.apply(this, args);

        // Log result and execution time
        if (options.result) {
          console.log(`[${className}.${propertyKey}] Returned:`, result);
        }
        if (options.timing) {
          console.log(`[${className}.${propertyKey}] Execution time: ${Date.now() - startTime}ms`);
        }

        return result;
      } catch (error) {
        console.error(`[${className}.${propertyKey}] Error:`, error);
        throw error;
      }
    };

    return descriptor;
  };
}

/**
 * Error Boundary Decorator
 * Handles method errors and provides fallback behavior
 * @param {Function} fallback - Fallback function to execute on error
 * @param {Function} errorHandler - Custom error handling function
 */
export function errorBoundary(fallback = null, errorHandler = null) {
  return function (target, propertyKey, descriptor) {
    const originalMethod = descriptor.value;

    descriptor.value = function (...args) {
      try {
        return originalMethod.apply(this, args);
      } catch (error) {
        // Handle error with custom handler if provided
        if (errorHandler) {
          errorHandler(error, propertyKey, this.constructor.name);
        } else {
          console.error(`Error in ${this.constructor.name}.${propertyKey}:`, error);
        }

        // Execute fallback if provided
        if (fallback) {
          return fallback.apply(this, args);
        }

        throw error; // Re-throw if no fallback provided
      }
    };

    return descriptor;
  };
}

// Usage example:
class ExampleService {
  @cache(10000)
  @log({ params: true, result: true, timing: true })
  @validate({
    userId: { required: true, type: 'number', min: 1 },
    data: { required: true, type: 'object' }
  })
  @rateLimit(5, 60000)
  @errorBoundary(
    function fallback(userId, data) { return { error: 'Fallback response' }; },
    function errorHandler(error, method, className) {
      console.error(`Custom error handler: ${className}.${method}`, error);
    }
  )
  async processUserData(userId, data) {
    // Method implementation
    return { userId, processedData: data };
  }
}