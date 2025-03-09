const express = require('express');
const rateLimit = require('express-rate-limit');
const NodeCache = require('node-cache');
const axios = require('axios');

const app = express();
const cache = new NodeCache();

const limiter = rateLimit({
    windowMs: 1 * 60 * 1000, // 1 minute
    max: 100, // Limit each IP to 100 requests per windowMs
    message: { error: 'Too many requests, please try again later.' }
});

const circuitBreakerState = {
    closed: true,
    failureCount: 0,
    successThreshold: 5,
    failureThreshold: 3,
    timeout: 30000, // 30 seconds
};
let lastFailureTime = null;

function circuitBreaker(req, res, next) {
    if (!circuitBreakerState.closed) {
        const currentTime = Date.now();
        if (currentTime - lastFailureTime > circuitBreakerState.timeout) {
            circuitBreakerState.closed = true;
            circuitBreakerState.failureCount = 0;
        } else {
            return res.status(503).json({ error: 'Service temporarily unavailable.' });
        }
    }
    next();
}

function handleFailure() {
    circuitBreakerState.failureCount++;
    if (circuitBreakerState.failureCount >= circuitBreakerState.failureThreshold) {
        circuitBreakerState.closed = false;
        lastFailureTime = Date.now();
    }
}

function cachingLayer(req, res, next) {
    const cacheKey = req.originalUrl;
    const cachedResponse = cache.get(cacheKey);
    if (cachedResponse) {
        return res.json(cachedResponse);
    }
    res.sendResponse = res.json;
    res.json = (body) => {
        cache.set(cacheKey, body, 60); // Cache for 60 seconds
        res.sendResponse(body);
    };
    next();
}

function authMiddleware(req, res, next) {
    const token = req.headers['authorization'];
    if (!token || token !== 'Bearer my-secret-token') {
        return res.status(403).json({ error: 'Unauthorized access.' });
    }
    next();
}

app.use(express.json());
app.use(requestValidation);
app.use(limiter);
app.use(circuitBreaker);
app.use(cachingLayer);
app.use(authMiddleware);

app.get('/api/data', async (req, res) => {
    try {
        const response = await axios.get('https://api.example.com/data');
        res.json(response.data);
        circuitBreakerState.failureCount = 0; // Reset failure count on success
    } catch (error) {
        handleFailure();
        res.status(500).json({ error: 'Failed to fetch data.' });
    }
});

const PORT = 3000;
app.listen(PORT, () => {
    console.log(`API Gateway running on http://localhost:${PORT}`);
});
