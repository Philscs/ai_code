const express = require('express');
const rateLimit = require('express-rate-limit');
const mcache = require('memory-cache');
const axios = require('axios');

const app = express();
const PORT = 3000;

// Dummy backend servers for load balancing
const backends = ['http://localhost:4000', 'http://localhost:4001'];
let backendIndex = 0;

// Middleware for Authentication
app.use((req, res, next) => {
  const apiKey = req.headers['x-api-key'];
  if (!apiKey || apiKey !== 'your-secure-api-key') {
    return res.status(403).json({ error: 'Unauthorized' });
  }
  next();
});

// Rate Limiting
const limiter = rateLimit({
  windowMs: 1 * 60 * 1000, // 1 Minute
  max: 10, // Limit each IP to 10 requests per window
  message: { error: 'Too many requests, please try again later.' },
});
app.use(limiter);

// Response Caching
const cache = (duration) => {
  return (req, res, next) => {
    const key = '__express__' + req.originalUrl || req.url;
    const cachedBody = mcache.get(key);
    if (cachedBody) {
      res.send(cachedBody);
      return;
    }
    res.sendResponse = res.send;
    res.send = (body) => {
      mcache.put(key, body, duration * 1000);
      res.sendResponse(body);
    };
    next();
  };
};

// Load Balancing Middleware
const loadBalancer = (req, res, next) => {
  backendIndex = (backendIndex + 1) % backends.length;
  req.backend = backends[backendIndex];
  next();
};

// Request Routing
app.use(loadBalancer);
app.use('/api/:service', cache(10), async (req, res) => {
  try {
    const serviceUrl = `${req.backend}/${req.params.service}`;
    const response = await axios({
      method: req.method,
      url: serviceUrl,
      data: req.body,
    });
    res.status(response.status).json(response.data);
  } catch (error) {
    res.status(error.response?.status || 500).json({ error: error.message });
  }
});

app.listen(PORT, () => {
  console.log(`API Gateway running on port ${PORT}`);
});
