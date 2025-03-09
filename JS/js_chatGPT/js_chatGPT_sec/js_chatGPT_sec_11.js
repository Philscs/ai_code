class Decorators {
    static methodCache(ttl = 10000) {
      const cache = new Map();
      return function (_, __, descriptor) {
        const originalMethod = descriptor.value;
        descriptor.value = function (...args) {
          const key = JSON.stringify(args);
          if (cache.has(key)) {
            const { value, expiry } = cache.get(key);
            if (Date.now() < expiry) {
              return value;
            } else {
              cache.delete(key);
            }
          }
          const result = originalMethod.apply(this, args);
          cache.set(key, { value: result, expiry: Date.now() + ttl });
          return result;
        };
        return descriptor;
      };
    }
  
    static rateLimit(limit = 1, interval = 1000) {
      return function (_, __, descriptor) {
        const originalMethod = descriptor.value;
        const timestamps = [];
  
        descriptor.value = function (...args) {
          const now = Date.now();
          timestamps.push(now);
          timestamps.filter((timestamp) => now - timestamp < interval);
  
          if (timestamps.length > limit) {
            throw new Error("Rate limit exceeded");
          }
  
          return originalMethod.apply(this, args);
        };
        return descriptor;
      };
    }
  
    static inputValidation(validationFn) {
      return function (_, __, descriptor) {
        const originalMethod = descriptor.value;
        descriptor.value = function (...args) {
          if (!validationFn(...args)) {
            throw new Error("Input validation failed");
          }
          return originalMethod.apply(this, args);
        };
        return descriptor;
      };
    }
  
    static logging(logFn = console.log) {
      return function (_, __, descriptor) {
        const originalMethod = descriptor.value;
        descriptor.value = function (...args) {
          logFn(`Method ${originalMethod.name} called with arguments:`, args);
          const result = originalMethod.apply(this, args);
          logFn(`Method ${originalMethod.name} returned:`, result);
          return result;
        };
        return descriptor;
      };
    }
  
    static errorBoundary(errorHandler = console.error) {
      return function (_, __, descriptor) {
        const originalMethod = descriptor.value;
        descriptor.value = function (...args) {
          try {
            return originalMethod.apply(this, args);
          } catch (error) {
            errorHandler(`Error in method ${originalMethod.name}:`, error);
            throw error;
          }
        };
        return descriptor;
      };
    }
  }
  
  // Beispiel-Nutzung
  class Example {
    @Decorators.methodCache(5000)
    expensiveCalculation(a, b) {
      return a + b; // Simuliert eine teure Berechnung
    }
  
    @Decorators.rateLimit(2, 1000)
    limitedAction() {
      console.log("Aktion ausgeführt");
    }
  
    @Decorators.inputValidation((a, b) => typeof a === "number" && typeof b === "number")
    addNumbers(a, b) {
      return a + b;
    }
  
    @Decorators.logging()
    loggableMethod(x) {
      return x * 2;
    }
  
    @Decorators.errorBoundary()
    riskyMethod() {
      throw new Error("Etwas ist schiefgelaufen!");
    }
  }
  
  // Beispiel
  const example = new Example();
  
  console.log(example.expensiveCalculation(1, 2)); // Caching
  example.limitedAction(); // Rate Limiting
  console.log(example.addNumbers(1, 2)); // Validation
  console.log(example.loggableMethod(5)); // Logging
  try {
    example.riskyMethod(); // Error Handling
  } catch (e) {
    console.error("Gefangene Ausnahme:", e);
  }
  