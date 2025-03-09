const express = require('express');
const app = express();
const cors = require('cors');
const auth = require('./auth.js');
const rateLimit = require('./rateLimit.js');
const cache = require('./cache.js');

// Request Routing
app.use(cors());
app.get('/api/:path*', (req, res) => {
  const path = req.params.path;
  return handleRequest(path);
});

function handleRequest(path) {
  if (path.startsWith('/api/auth')) {
    // Auth-Route
    return auth.handleAuth(req, res);
  } else if (path.startsWith('/api/rate-limit')) {
    // Rate-Limit-Routing
    return rateLimit.handleRateLimit(req, res);
  }
}

// Response Caching
app.use(cache.cacheResponse);

function cacheResponse(req, res) {
  const path = req.url;
  const cachedResponse = cache.get(path);
  if (cachedResponse) {
    return res.status(200).send(cachedResponse);
  } else {
    // Führen der Request-Handling-Logic
    handleRequest(req.url);
  }
}

// Load Balancing
const loadBalancer = require('./loadBalancer.js');
app.use(loadBalancer.loadBalance(req, res));

function loadBalance(req, res) {
  const path = req.url;
  if (path.startsWith('/api/auth')) {
    // Auth-Route
    return auth.handleAuth(req, res);
  } else if (path.startsWith('/api/rate-limit')) {
    // Rate-Limit-Routing
    return rateLimit.handleRateLimit(req, res);
  }
}

// Authentication
app.use(auth.verifyToken);

function verifyToken(req, res) {
  const token = req.headers['authorization'];
  if (!token) {
    return res.status(401).send('Token not provided');
  }
  auth.verify(token, (err, user) => {
    if (err) {
      return res.status(500).send(err);
    }
    req.user = user;
    handleRequest(req.url);
  });
}

// Rate Limiting
app.use(rateLimit.limitRequests());

function limitRequests(req, res) {
  const path = req.url;
  rateLimit.limitRequests(path, (err, count) => {
    if (err) {
      return res.status(500).send(err);
    }
    if (count < 10) {
      // Wenn es innerhalb der Rate-Limit-Fenster ist
      handleRequest(req.url);
    } else {
      // Wenn die Rate-Limit-Ebene überschritten wurde
      return res.status(429).send('Rate Limit exceeded');
    }
  });
}

// Server-Start
const port = process.env.PORT || 3000;
app.listen(port, () => {
  console.log(`Server gestartet auf Port ${port}`);
});