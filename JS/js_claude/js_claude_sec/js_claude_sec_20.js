// Schema definition using a simple type system
class RPCSchema {
    constructor() {
      this.methods = new Map();
    }
  
    // Register method with parameter and return types
    defineMethod(name, paramSchema, returnSchema) {
      this.methods.set(name, { paramSchema, returnSchema });
    }
  
    // Validate data against schema
    validate(schema, data) {
      if (schema.type === 'object') {
        if (typeof data !== 'object' || data === null) return false;
        return Object.entries(schema.properties).every(([key, propSchema]) =>
          this.validate(propSchema, data[key])
        );
      }
      if (schema.type === 'array') {
        if (!Array.isArray(data)) return false;
        return data.every(item => this.validate(schema.items, item));
      }
      return typeof data === schema.type;
    }
  }
  
  // RPC Client implementation
  class RPCClient {
    constructor(url, schema) {
      this.url = url;
      this.schema = schema;
      this.ws = null;
      this.requestId = 0;
      this.pendingRequests = new Map();
      this.rateLimiter = new RateLimiter(100, 60000); // 100 requests per minute
    }
  
    // Connect to the server with transport security
    async connect() {
      return new Promise((resolve, reject) => {
        this.ws = new WebSocket(this.url, {
          rejectUnauthorized: true, // Require valid SSL certificate
          cert: process.env.CLIENT_CERT,
          key: process.env.CLIENT_KEY,
        });
  
        this.ws.onopen = () => {
          console.log('Connected to RPC server');
          resolve();
        };
  
        this.ws.onerror = (error) => {
          console.error('WebSocket error:', error);
          reject(error);
        };
  
        this.ws.onmessage = (event) => {
          this.handleResponse(JSON.parse(event.data));
        };
      });
    }
  
    // Make RPC call with schema validation
    async call(method, params) {
      if (!this.rateLimiter.checkLimit()) {
        throw new Error('Rate limit exceeded');
      }
  
      const methodSchema = this.schema.methods.get(method);
      if (!methodSchema) {
        throw new Error(`Unknown method: ${method}`);
      }
  
      if (!this.schema.validate(methodSchema.paramSchema, params)) {
        throw new Error('Invalid parameters');
      }
  
      const requestId = this.requestId++;
      const request = {
        jsonrpc: '2.0',
        id: requestId,
        method,
        params,
      };
  
      return new Promise((resolve, reject) => {
        this.pendingRequests.set(requestId, { resolve, reject });
        this.ws.send(JSON.stringify(request));
      });
    }
  
    // Handle responses and validate return types
    handleResponse(response) {
      const { id, result, error } = response;
      const pending = this.pendingRequests.get(id);
      if (!pending) return;
  
      this.pendingRequests.delete(id);
  
      if (error) {
        pending.reject(new RPCError(error.code, error.message));
        return;
      }
  
      const methodSchema = this.schema.methods.get(response.method);
      if (!this.schema.validate(methodSchema.returnSchema, result)) {
        pending.reject(new Error('Invalid response type'));
        return;
      }
  
      pending.resolve(result);
    }
  }
  
  // RPC Server implementation
  class RPCServer {
    constructor(schema) {
      this.schema = schema;
      this.methods = new Map();
      this.rateLimiter = new Map(); // Client ID -> RateLimiter
    }
  
    // Register method implementation
    registerMethod(name, handler) {
      if (!this.schema.methods.has(name)) {
        throw new Error(`Method ${name} not defined in schema`);
      }
      this.methods.set(name, handler);
    }
  
    // Start server with secure WebSocket
    start(port) {
      const wss = new WebSocket.Server({
        port,
        cert: process.env.SERVER_CERT,
        key: process.env.SERVER_KEY,
      });
  
      wss.on('connection', (ws, req) => {
        const clientId = req.connection.remoteAddress;
        if (!this.rateLimiter.has(clientId)) {
          this.rateLimiter.set(clientId, new RateLimiter(100, 60000));
        }
  
        ws.on('message', async (message) => {
          try {
            const request = JSON.parse(message);
            await this.handleRequest(ws, clientId, request);
          } catch (error) {
            this.sendError(ws, request?.id, -32700, 'Parse error');
          }
        });
      });
    }
  
    // Handle incoming requests with validation and rate limiting
    async handleRequest(ws, clientId, request) {
      const { jsonrpc, id, method, params } = request;
  
      if (jsonrpc !== '2.0') {
        return this.sendError(ws, id, -32600, 'Invalid Request');
      }
  
      if (!this.rateLimiter.get(clientId).checkLimit()) {
        return this.sendError(ws, id, -32000, 'Rate limit exceeded');
      }
  
      const methodSchema = this.schema.methods.get(method);
      if (!methodSchema) {
        return this.sendError(ws, id, -32601, 'Method not found');
      }
  
      if (!this.schema.validate(methodSchema.paramSchema, params)) {
        return this.sendError(ws, id, -32602, 'Invalid params');
      }
  
      try {
        const handler = this.methods.get(method);
        const result = await handler(params);
  
        if (!this.schema.validate(methodSchema.returnSchema, result)) {
          return this.sendError(ws, id, -32603, 'Invalid response type');
        }
  
        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          id,
          result,
        }));
      } catch (error) {
        this.sendError(ws, id, -32000, error.message);
      }
    }
  
    // Send error response
    sendError(ws, id, code, message) {
      ws.send(JSON.stringify({
        jsonrpc: '2.0',
        id,
        error: { code, message },
      }));
    }
  }
  
  // Rate limiting implementation
  class RateLimiter {
    constructor(limit, window) {
      this.limit = limit;
      this.window = window;
      this.requests = [];
    }
  
    checkLimit() {
      const now = Date.now();
      this.requests = this.requests.filter(time => now - time < this.window);
      
      if (this.requests.length >= this.limit) {
        return false;
      }
  
      this.requests.push(now);
      return true;
    }
  }
  
  // Custom error class for RPC errors
  class RPCError extends Error {
    constructor(code, message) {
      super(message);
      this.code = code;
      this.name = 'RPCError';
    }
  }
  
  // Example usage:
  const schema = new RPCSchema();
  
  // Define method schemas
  schema.defineMethod('add', {
    type: 'object',
    properties: {
      a: { type: 'number' },
      b: { type: 'number' }
    }
  }, {
    type: 'number'
  });
  
  // Create server
  const server = new RPCServer(schema);
  server.registerMethod('add', async ({ a, b }) => a + b);
  server.start(8080);
  
  // Create client
  const client = new RPCClient('wss://localhost:8080', schema);
  await client.connect();
  
  // Make RPC call
  try {
    const result = await client.call('add', { a: 5, b: 3 });
    console.log('Result:', result); // 8
  } catch (error) {
    console.error('RPC error:', error);
  }