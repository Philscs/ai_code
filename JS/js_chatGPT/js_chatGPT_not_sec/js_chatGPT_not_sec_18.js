// Import notwendige Bibliotheken
const express = require('express');
const Redis = require('ioredis');
const app = express();
const redis = new Redis(); // Redis Verbindung, um verteilte Limits zu unterstützen

// Konfiguration
const config = {
    ipLimit: { capacity: 100, refillRate: 10 },
    userLimit: { capacity: 200, refillRate: 20 },
    decayInterval: 1000, // Millisekunden
};

// Token-Bucket-Algorithmus für einen Schlüssel
async function tokenBucket(key, capacity, refillRate, interval) {
    const now = Date.now();

    // Hole bestehende Daten aus Redis
    const data = await redis.hmget(key, 'tokens', 'lastRefill');
    let tokens = parseFloat(data[0]) || capacity;
    const lastRefill = parseFloat(data[1]) || now;

    // Tokens entsprechend der vergangenen Zeit auffüllen
    const elapsed = now - lastRefill;
    tokens = Math.min(capacity, tokens + (elapsed / interval) * refillRate);

    // Token abziehen und Rate-Limit überprüfen
    if (tokens < 1) {
        return false;
    }
    tokens -= 1;

    // Aktualisiere den Token-Status in Redis
    await redis.hmset(key, {
        tokens: tokens,
        lastRefill: now,
    });
    redis.expire(key, Math.ceil(interval / 1000) * 2); // Schlüssel-TTL setzen

    return true;
}

// Middleware für Rate-Limiting
async function rateLimiter(req, res, next) {
    const ip = req.ip;
    const user = req.user?.id; // Benutzer-ID, falls vorhanden

    try {
        // IP-basierte Rate-Limits prüfen
        const ipKey = `rate_limit:ip:${ip}`;
        const ipAllowed = await tokenBucket(
            ipKey,
            config.ipLimit.capacity,
            config.ipLimit.refillRate,
            config.decayInterval
        );

        if (!ipAllowed) {
            return res.status(429).json({ error: 'Rate limit exceeded for IP.' });
        }

        // Benutzer*innen-basierte Rate-Limits prüfen (falls Benutzer*innen authentifiziert sind)
        if (user) {
            const userKey = `rate_limit:user:${user}`;
            const userAllowed = await tokenBucket(
                userKey,
                config.userLimit.capacity,
                config.userLimit.refillRate,
                config.decayInterval
            );

            if (!userAllowed) {
                return res.status(429).json({ error: 'Rate limit exceeded for user.' });
            }
        }

        next();
    } catch (err) {
        console.error('Rate limiter error:', err);

        // Graceful Degradation: Weiterleitung ohne Einschränkungen bei Fehlern
        next();
    }
}

// Beispiel-Routen
app.use(rateLimiter);
app.get('/', (req, res) => {
    res.send('Willkommen! Sie haben Zugriff auf diese API.');
});

// Server starten
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`Server läuft auf Port ${PORT}`);
});
