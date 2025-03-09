// Core Gateway class
class APIGateway {
    constructor() {
      this.middlewares = [];
      this.cache = new Map();
      this.rateLimiter = new RateLimiter();
      this.circuitBreaker = new CircuitBreaker();
      this.authManager = new AuthManager();
      this.validator = new RequestValidator();
    }
  
    use(middleware) {
      this.middlewares.push(middleware);
    }
  
    async handleRequest(request) {
      try {
        // Apply middlewares in sequence
        for (const middleware of this.middlewares) {
          await middleware(request);
        }
  
        // Validate request
        await this.validator.validate(request);
  
        // Check rate limit
        await this.rateLimiter.checkLimit(request);
  
        // Check authentication
        await this.authManager.authenticate(request);
  
        // Check cache
        const cachedResponse = await this.checkCache(request);
        if (cachedResponse) {
          return cachedResponse;
        }
  
        // Forward request through circuit breaker
        const response = await this.circuitBreaker.forward(request);
  
        // Cache response
        await this.cacheResponse(request, response);
  
        return response;
      } catch (error) {
        return this.handleError(error);
      }
    }
  
    async checkCache(request) {
      const key = this.getCacheKey(request);
      return this.cache.get(key);
    }
  
    async cacheResponse(request, response) {
      const key = this.getCacheKey(request);
      this.cache.set(key, response);
    }
  
    getCacheKey(request) {
      return `${request.method}:${request.path}:${JSON.stringify(request.params)}`;
    }
  
    handleError(error) {
      return {
        status: error.status || 500,
        message: error.message || 'Internal Server Error'
      };
    }
  }
  
  // Request Validator
  class RequestValidator {
    constructor() {
      this.schemas = new Map();
    }
  
    addSchema(path, schema) {
      this.schemas.set(path, schema);
    }
  
    async validate(request) {
      const schema = this.schemas.get(request.path);
      if (!schema) return;
  
      const validation = schema.validate(request.body);
      if (validation.error) {
        throw {
          status: 400,
          message: 'Invalid request',
          details: validation.error.details
        };
      }
    }
  }
  
  // Rate Limiter
  class RateLimiter {
    constructor(limit = 100, windowMs = 60000) {
      this.limit = limit;
      this.windowMs = windowMs;
      this.requests = new Map();
    }
  
    async checkLimit(request) {
      const key = this.getClientKey(request);
      const now = Date.now();
      const windowStart = now - this.windowMs;
  
      let clientRequests = this.requests.get(key) || [];
      clientRequests = clientRequests.filter(timestamp => timestamp > windowStart);
  
      if (clientRequests.length >= this.limit) {
        throw {
          status: 429,
          message: 'Too Many Requests'
        };
      }
  
      clientRequests.push(now);
      this.requests.set(key, clientRequests);
    }
  
    getClientKey(request) {
      return request.ip || request.headers['x-forwarded-for'];
    }
  }
  
  // Circuit Breaker
  class CircuitBreaker {
    constructor(failureThreshold = 5, resetTimeout = 60000) {
      this.state = 'CLOSED';
      this.failureCount = 0;
      this.failureThreshold = failureThreshold;
      this.resetTimeout = resetTimeout;
      this.lastFailureTime = null;
    }
  
    async forward(request) {
      if (this.state === 'OPEN') {
        if (Date.now() - this.lastFailureTime >= this.resetTimeout) {
          this.state = 'HALF_OPEN';
        } else {
          throw {
            status: 503,
            message: 'Service Temporarily Unavailable'
          };
        }
      }
  
      try {
        const response = await this.makeRequest(request);
        
        if (this.state === 'HALF_OPEN') {
          this.state = 'CLOSED';
          this.failureCount = 0;
        }
  
        return response;
      } catch (error) {
        this.handleFailure();
        throw error;
      }
    }
  
    async makeRequest(request) {
      // Implement actual request forwarding logic here
      return fetch(request.url, {
        method: request.method,
        headers: request.headers,
        body: JSON.stringify(request.body)
      });
    }
  
    handleFailure() {
      this.failureCount++;
      if (this.failureCount >= this.failureThreshold) {
        this.state = 'OPEN';
        this.lastFailureTime = Date.now();
      }
    }
  }
  
  // Auth Manager
  class AuthManager {
    constructor() {
      this.roles = new Map();
      this.users = new Map();
    }
  
    async authenticate(request) {
      const token = this.extractToken(request);
      if (!token) {
        throw {
          status: 401,
          message: 'Unauthorized'
        };
      }
  
      const user = await this.validateToken(token);
      if (!user) {
        throw {
          status: 401,
          message: 'Invalid token'
        };
      }
  
      request.user = user;
    }
  
    async authorize(request, requiredRole) {
      if (!request.user) {
        throw {
          status: 401,
          message: 'Unauthorized'
        };
      }
  
      const userRole = this.roles.get(request.user.id);
      if (!userRole || userRole !== requiredRole) {
        throw {
          status: 403,
          message: 'Forbidden'
        };
      }
    }
  
    extractToken(request) {
      const authHeader = request.headers.authorization;
      if (!authHeader || !authHeader.startsWith('Bearer ')) {
        return null;
      }
      return authHeader.substring(7);
    }
  
    async validateToken(token) {
      // Implement JWT validation or other token validation logic
      return null;
    }
  }
  
  // Usage Example
  const gateway = new APIGateway();
  
  // Add middleware
  gateway.use(async (request) => {
    request.startTime = Date.now();
  });
  
  // Add request schema
  const userSchema = {
    validate: (data) => {
      // Implement validation logic
      return { error: null };
    }
  };
  gateway.validator.addSchema('/users', userSchema);
  
  // Handle request
  const request = {
    method: 'POST',
    path: '/users',
    body: { /* request body */ },
    headers: {
      authorization: 'Bearer token'
    }
  };
  
  gateway.handleRequest(request)
    .then(response => console.log('Response:', response))
    .catch(error => console.error('Error:', error));
  
  export default APIGateway;