
// Method Caching Decorator
function cache(fn) {
  const cache = new Map();
  return function (...args) {
    const key = JSON.stringify(args);
    if (cache.has(key)) {
      return cache.get(key);
    }
    const result = fn.apply(this, args);
    cache.set(key, result);
    return result;
  };
}

// Rate Limiting Decorator
function rateLimit(limit) {
  let count = 0;
  let lastCalledTime = 0;
  return function (fn) {
    return function (...args) {
      const now = Date.now();
      if (now - lastCalledTime < limit) {
        console.log('Rate limit exceeded');
        return;
      }
      lastCalledTime = now;
      count++;
      return fn.apply(this, args);
    };
  };
}

// Input Validation Decorator
function validate(validationFn) {
  return function (fn) {
    return function (...args) {
      if (!validationFn(...args)) {
        console.log('Input validation failed');
        return;
      }
      return fn.apply(this, args);
    };
  };
}

// Logging Decorator
function log(fn) {
  return function (...args) {
    console.log(`Calling function ${fn.name} with arguments:`, args);
    const result = fn.apply(this, args);
    console.log(`Function ${fn.name} returned:`, result);
    return result;
  };
}

// Error Boundary Decorator
function errorBoundary(fn) {
  return function (...args) {
    try {
      return fn.apply(this, args);
    } catch (error) {
      console.error('An error occurred:', error);
    }
  };
}

// Usage example
class ExampleClass {
  @cache
  @rateLimit(1000)
  @validate((arg) => arg > 0)
  @log
  @errorBoundary
  exampleMethod(arg) {
    console.log('Executing exampleMethod with argument:', arg);
    return arg * 2;
  }
}

const exampleInstance = new ExampleClass();
exampleInstance.exampleMethod(5); // Output: 10 (method executed)
exampleInstance.exampleMethod(5); // Output: 10 (cached result)
exampleInstance.exampleMethod(-1); // Output: "Input validation failed"
exampleInstance.exampleMethod(10); // Output: "Rate limit exceeded"
