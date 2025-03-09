const EventEmitter = require('events');

class ReactiveCache extends EventEmitter {
    constructor(options = {}) {
        super();
        this.cache = new Map(); // Simple in-memory cache
        this.ttl = options.ttl || 60000; // Default TTL 60 seconds
        this.maxSize = options.maxSize || 100; // Max cache entries
        this.securityHeaders = options.securityHeaders || {
            'Content-Security-Policy': "default-src 'self'",
            'X-Content-Type-Options': 'nosniff',
        };
        this.dosLimit = options.dosLimit || 100; // Requests per second
        this.requestCounts = new Map(); // Track requests for DOS prevention
        this.init();
    }

    init() {
        // Periodically clean up expired cache entries
        setInterval(() => this.cleanUp(), this.ttl);
        // Reset DOS counters periodically
        setInterval(() => this.resetDosCounters(), 1000);
    }

    // Cache warmup
    warmup(key, fetchDataFn) {
        if (!this.cache.has(key)) {
            fetchDataFn().then((data) => {
                this.set(key, data);
            }).catch((err) => {
                console.error('Error during cache warmup:', err);
            });
        }
    }

    // Cache invalidation
    invalidate(key) {
        this.cache.delete(key);
        this.emit('invalidate', key);
    }

    // Set cache entry with expiration
    set(key, value) {
        if (this.cache.size >= this.maxSize) {
            this.evict();
        }
        const expiresAt = Date.now() + this.ttl;
        this.cache.set(key, { value, expiresAt });
        this.emit('set', key, value);
    }

    // Get cache entry
    get(key) {
        const entry = this.cache.get(key);
        if (entry && entry.expiresAt > Date.now()) {
            return entry.value;
        } else {
            this.invalidate(key);
            return null;
        }
    }

    // Clean up expired entries
    cleanUp() {
        const now = Date.now();
        for (const [key, { expiresAt }] of this.cache.entries()) {
            if (expiresAt <= now) {
                this.invalidate(key);
            }
        }
    }

    // Evict least recently used item (simple FIFO for this example)
    evict() {
        const firstKey = this.cache.keys().next().value;
        if (firstKey) {
            this.invalidate(firstKey);
        }
    }

    // Add security headers to HTTP responses
    applySecurityHeaders(response) {
        for (const [header, value] of Object.entries(this.securityHeaders)) {
            response.setHeader(header, value);
        }
    }

    // DOS Prevention Middleware
    dosPreventionMiddleware(req, res, next) {
        const ip = req.ip || req.connection.remoteAddress;
        const count = this.requestCounts.get(ip) || 0;

        if (count >= this.dosLimit) {
            res.status(429).send('Too many requests');
            return;
        }

        this.requestCounts.set(ip, count + 1);
        next();
    }

    // Reset DOS counters periodically
    resetDosCounters() {
        this.requestCounts.clear();
    }
}

// Example usage
const cache = new ReactiveCache({ ttl: 30000, maxSize: 50 });

// Event listeners
cache.on('set', (key, value) => {
    console.log(`Cache set: ${key} = ${value}`);
});
cache.on('invalidate', (key) => {
    console.log(`Cache invalidated: ${key}`);
});

// Cache warmup example
cache.warmup('exampleKey', async () => {
    return await fetchDataFromDatabaseOrAPI();
});

// Middleware for Express.js
const express = require('express');
const app = express();

app.use((req, res, next) => cache.dosPreventionMiddleware(req, res, next));
app.use((req, res, next) => {
    cache.applySecurityHeaders(res);
    next();
});

app.get('/data/:key', (req, res) => {
    const key = req.params.key;
    const value = cache.get(key);

    if (value) {
        res.send(value);
    } else {
        res.status(404).send('Not Found');
    }
});

app.listen(3000, () => console.log('Server running on port 3000'));

async function fetchDataFromDatabaseOrAPI() {
    // Simulated async data fetch
    return new Promise((resolve) => setTimeout(() => resolve('Fetched Data'), 100));
}
