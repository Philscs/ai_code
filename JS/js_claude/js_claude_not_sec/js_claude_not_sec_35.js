const express = require('express');
const rateLimit = require('express-rate-limit');
const jwt = require('jsonwebtoken');
const NodeCache = require('node-cache');
const axios = require('axios');

class APIGateway {
    constructor() {
        this.app = express();
        this.cache = new NodeCache({ stdTTL: 600 }); // 10 minutes default TTL
        this.services = new Map();
        this.currentServiceIndex = 0;

        // Middleware Setup
        this.setupMiddleware();
    }

    setupMiddleware() {
        this.app.use(express.json());
        this.app.use(this.authenticateRequest.bind(this));
        this.app.use(this.setupRateLimiting());
    }

    // Authentication Middleware
    async authenticateRequest(req, res, next) {
        const token = req.headers.authorization?.split(' ')[1];
        
        if (!token) {
            return res.status(401).json({ error: 'No token provided' });
        }

        try {
            const decoded = jwt.verify(token, process.env.JWT_SECRET);
            req.user = decoded;
            next();
        } catch (error) {
            return res.status(401).json({ error: 'Invalid token' });
        }
    }

    // Rate Limiting
    setupRateLimiting() {
        return rateLimit({
            windowMs: 15 * 60 * 1000, // 15 minutes
            max: 100, // Limit each IP to 100 requests per windowMs
            message: 'Too many requests from this IP, please try again later.'
        });
    }

    // Response Caching
    getCacheKey(req) {
        return `${req.method}:${req.path}:${JSON.stringify(req.query)}:${JSON.stringify(req.body)}`;
    }

    async handleCaching(req, res, next) {
        const cacheKey = this.getCacheKey(req);
        const cachedResponse = this.cache.get(cacheKey);

        if (cachedResponse) {
            return res.json(cachedResponse);
        }

        res.originalJson = res.json;
        res.json = (body) => {
            this.cache.set(cacheKey, body);
            res.originalJson(body);
        };

        next();
    }

    // Load Balancing
    registerService(serviceName, endpoints) {
        this.services.set(serviceName, endpoints);
    }

    getNextEndpoint(serviceName) {
        const endpoints = this.services.get(serviceName);
        if (!endpoints || endpoints.length === 0) {
            throw new Error(`No endpoints registered for service: ${serviceName}`);
        }

        // Round-robin selection
        const endpoint = endpoints[this.currentServiceIndex % endpoints.length];
        this.currentServiceIndex++;
        return endpoint;
    }

    // Request Routing
    async routeRequest(serviceName, req, res) {
        try {
            const endpoint = this.getNextEndpoint(serviceName);
            const response = await axios({
                method: req.method,
                url: `${endpoint}${req.path}`,
                data: req.body,
                headers: {
                    ...req.headers,
                    host: new URL(endpoint).host,
                },
                params: req.query
            });

            return res.status(response.status).json(response.data);
        } catch (error) {
            console.error(`Error routing request to ${serviceName}:`, error);
            return res.status(error.response?.status || 500).json({
                error: 'Service unavailable',
                details: error.message
            });
        }
    }

    // Route Registration
    registerRoute(method, path, serviceName) {
        this.app[method.toLowerCase()](path, 
            this.handleCaching.bind(this),
            (req, res) => this.routeRequest(serviceName, req, res)
        );
    }

    // Start the gateway
    start(port) {
        this.app.listen(port, () => {
            console.log(`API Gateway listening on port ${port}`);
        });
    }
}

// Example usage:
const gateway = new APIGateway();

// Register services with their endpoints
gateway.registerService('userService', [
    'http://user-service-1:3001',
    'http://user-service-2:3001'
]);

gateway.registerService('orderService', [
    'http://order-service-1:3002',
    'http://order-service-2:3002'
]);

// Register routes
gateway.registerRoute('GET', '/users', 'userService');
gateway.registerRoute('POST', '/users', 'userService');
gateway.registerRoute('GET', '/orders', 'orderService');
gateway.registerRoute('POST', '/orders', 'orderService');

// Start the gateway
gateway.start(3000);