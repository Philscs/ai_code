// Importiere die benötigten Bibliotheken
const redis = require('redis');
const moment = require('moment');

class RequestRateLimiter {
  constructor(ip, user) {
    this.ip = ip;
    this.user = user;
    this.tokenBucket = new TokenBucket(60, 5); // 1 Minute Zeitfenster mit 5 Tokens
    this.distributedTokens = []; // Token-Liste für verteiltes Rate Begrenzung

    // Verbindung zur Redis-Datenbank aufbauen
    this.redisClient = redis.createClient({
      host: 'localhost',
      port: 6379,
      password: null,
    });

    // Funktion, die bei jedem Request aufgerufen wird
    this.onRequest = (req) => {
      if (!this.tokenBucket.hasTokens()) {
        return { success: false, message: 'Rate Limit erreicht' };
      }

      const tokens = this.tokenBucket.consumeToken();
      this.distributedTokens.push({ timestamp: moment(), tokens });

      // Verteilte Rate Begrenzung implementieren
      if (req.ip !== this.ip) {
        const existingTokens = this.redisClient.get(`distributed-tokens:${this.user}`);
        if (existingTokens && existingTokens > 0) {
          const newTokens = Math.min(existingTokens - 1, tokens);
          this.redisClient.set(`distributed-tokens:${this.user}`, newTokens);
        } else {
          this.redisClient.set(`distributed-tokens:${this.user}`, tokens);
        }
      }

      return { success: true };
    };

    // Funktion, die bei Token-Expirieren aufgerufen wird
    this.onTokenExpire = (timestamp) => {
      const index = this.distributedTokens.findIndex((token) => token.timestamp.getTime() === 
timestamp);
      if (index !== -1) {
        delete this.distributedTokens[index];
        return;
      }

      // Lösche alle verstricketen Tokens
      while (this.distributedTokens.length > 0 && this.distributedTokens[0].timestamp.getTime() < 
timestamp - 30000) {
        const index = this.distributedTokens.findIndex((token) => token.timestamp.getTime() === 
this.distributedTokens[0].timestamp.getTime());
        delete this.distributedTokens[index];
      }
    };
  }

  start() {
    // Aufgerufen, wenn der Rate Limiter gestartet wird
    setInterval(() => {
      this.redisClient.del(`distributed-tokens:${this.user}`);
    }, 30000);

    // Aufgerufen, wenn ein Request aufgerufen wird
    req.on('request', (req) => {
      const result = this.onRequest(req);
      res.json(result);
    });

    // Aufgerufen, wenn ein Token abläuft
    this.tokenBucket.onTokenExpire = (timestamp) => {
      this.onTokenExpire(timestamp);
    };
  }
}

class TokenBucket {
  constructor(refreshInterval, initialTokens) {
    this.refreshInterval = refreshInterval;
    this.initialTokens = initialTokens;
    this.tokens = initialTokens;
    this.lastRefreshTime = Date.now();

    // Verbindung zur Redis-Datenbank aufbauen
    const redisClient = require('redis').createClient({
      host: 'localhost',
      port: 6379,
      password: null,
    });

    // Funktion, die bei jedem Refresh-Timer aufgerufen wird
    setInterval(() => {
      if (this.tokens > 0) {
        this.refreshTokens();
      }
    }, refreshInterval);

    // Funktion, die bei Token-Refresh aufgerufen wird
    redisClient.on('token refreshed', () => {
      this.refreshTokens();
    });
  }

  refreshTokens() {
    const now = Date.now();
    if (this.lastRefreshTime + this.refreshInterval * 1000 < now) {
      this.tokens = Math.min(this.initialTokens, this.tokens + now - this.lastRefreshTime);
      this.lastRefreshTime = now;
    }
  }

  consumeToken() {
    while (this.tokens > 0 && Date.now() - this.lastRefreshTime >= this.refreshInterval * 1000) {
      this.tokens--;
      this.lastRefreshTime = Date.now();
    }

    return this.tokens;
  }

  hasTokens() {
    return this.tokens > 0;
  }
}

// Beispiel für die Verwendung des Rate Limiters
const limiter = new RequestRateLimiter('192.168.1.100', 'user123');
limiter.start();

res.json({ message: 'Request erfolgreich' });
