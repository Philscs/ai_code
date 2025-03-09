class TokenBucket {
    constructor(capacity, refillRate) {
        this.capacity = capacity;
        this.refillRate = refillRate;
        this.tokens = capacity;
        this.lastRefill = Date.now();
    }

    refill() {
        const now = Date.now();
        const timePassed = (now - this.lastRefill) / 1000;
        this.tokens = Math.min(
            this.capacity,
            this.tokens + timePassed * this.refillRate
        );
        this.lastRefill = now;
    }

    tryConsume(tokens = 1) {
        this.refill();
        if (this.tokens >= tokens) {
            this.tokens -= tokens;
            return true;
        }
        return false;
    }
}

class DistributedStorage {
    constructor() {
        // In a real implementation, this would be Redis or similar
        this.storage = new Map();
    }

    async get(key) {
        return this.storage.get(key);
    }

    async set(key, value, ttl = 60) {
        this.storage.set(key, value);
        setTimeout(() => this.storage.delete(key), ttl * 1000);
    }

    async increment(key, value = 1) {
        const current = this.storage.get(key) || 0;
        const newValue = current + value;
        this.storage.set(key, newValue);
        return newValue;
    }
}

class RateLimiter {
    constructor(options = {}) {
        this.storage = new DistributedStorage();
        this.options = {
            ipLimit: options.ipLimit || 100,
            userLimit: options.userLimit || 200,
            windowMs: options.windowMs || 60000,
            bucketCapacity: options.bucketCapacity || 50,
            bucketRefillRate: options.bucketRefillRate || 10,
            ...options
        };

        // Keep track of token buckets
        this.buckets = new Map();
    }

    async isRateLimited(request) {
        const ip = request.ip;
        const userId = request.userId;

        try {
            // Check IP-based limit
            const ipKey = `ratelimit:ip:${ip}`;
            const ipRequests = await this.storage.increment(ipKey);
            await this.storage.set(ipKey, ipRequests, this.options.windowMs / 1000);

            if (ipRequests > this.options.ipLimit) {
                return {
                    limited: true,
                    reason: 'IP_LIMIT_EXCEEDED',
                    retryAfter: this.getRetryAfter()
                };
            }

            // Check user-based limit if userId exists
            if (userId) {
                const userKey = `ratelimit:user:${userId}`;
                const userRequests = await this.storage.increment(userKey);
                await this.storage.set(userKey, userRequests, this.options.windowMs / 1000);

                if (userRequests > this.options.userLimit) {
                    return {
                        limited: true,
                        reason: 'USER_LIMIT_EXCEEDED',
                        retryAfter: this.getRetryAfter()
                    };
                }
            }

            // Check token bucket
            const bucketKey = userId || ip;
            if (!this.buckets.has(bucketKey)) {
                this.buckets.set(
                    bucketKey,
                    new TokenBucket(
                        this.options.bucketCapacity,
                        this.options.bucketRefillRate
                    )
                );
            }

            const bucket = this.buckets.get(bucketKey);
            if (!bucket.tryConsume()) {
                return {
                    limited: true,
                    reason: 'BURST_LIMIT_EXCEEDED',
                    retryAfter: this.getRetryAfter()
                };
            }

            return { limited: false };
        } catch (error) {
            // Graceful degradation: Allow request through if rate limiting fails
            console.error('Rate limiting failed:', error);
            return { limited: false, degraded: true };
        }
    }

    getRetryAfter() {
        return Math.ceil(this.options.windowMs / 1000);
    }
}

// Example middleware for Express.js
function rateLimiterMiddleware(options) {
    const limiter = new RateLimiter(options);

    return async (req, res, next) => {
        try {
            const result = await limiter.isRateLimited({
                ip: req.ip,
                userId: req.user?.id // Assuming user object is attached to request
            });

            if (result.limited) {
                res.set('Retry-After', result.retryAfter);
                res.status(429).json({
                    error: 'Too Many Requests',
                    reason: result.reason,
                    retryAfter: result.retryAfter
                });
                return;
            }

            if (result.degraded) {
                // Log degraded service for monitoring
                console.warn('Rate limiting degraded, allowing request');
            }

            next();
        } catch (error) {
            next(error);
        }
    };
}

// Usage example
const app = express();

app.use(
    rateLimiterMiddleware({
        ipLimit: 100,              // 100 requests per IP per window
        userLimit: 200,            // 200 requests per user per window
        windowMs: 60000,           // 1 minute window
        bucketCapacity: 50,        // Maximum burst of 50 tokens
        bucketRefillRate: 10       // Refill 10 tokens per second
    })
);