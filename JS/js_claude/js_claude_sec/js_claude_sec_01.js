// auth.js
const express = require('express');
const jwt = require('jsonwebtoken');
const crypto = require('crypto');
const rateLimit = require('express-rate-limit');
const helmet = require('helmet');
const cookieParser = require('cookie-parser');

// Konfiguration
const config = {
  accessTokenSecret: crypto.randomBytes(32).toString('hex'),
  refreshTokenSecret: crypto.randomBytes(32).toString('hex'),
  accessTokenExpiry: '15m',
  refreshTokenExpiry: '7d',
  cookieMaxAge: 7 * 24 * 60 * 60 * 1000, // 7 Tage
};

// In-Memory Token Blacklist (In Produktion: Redis verwenden)
const tokenBlacklist = new Set();

// Express App Setup
const app = express();

// Sicherheits-Middleware
app.use(helmet());
app.use(cookieParser());
app.use(express.json());

// CSRF Token Middleware
app.use((req, res, next) => {
  if (req.method === 'GET') {
    res.cookie('XSRF-TOKEN', crypto.randomBytes(32).toString('hex'), {
      httpOnly: false,
      secure: true,
      sameSite: 'strict'
    });
  }
  next();
});

// Rate Limiting
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 Minuten
  max: 100 // max 100 Requests pro IP
});
app.use('/api/', limiter);

// Token Generation
const generateAccessToken = (user) => {
  return jwt.sign({ id: user.id, role: user.role }, config.accessTokenSecret, {
    expiresIn: config.accessTokenExpiry
  });
};

const generateRefreshToken = (user) => {
  return jwt.sign({ id: user.id }, config.refreshTokenSecret, {
    expiresIn: config.refreshTokenExpiry
  });
};

// Middleware für Token-Validierung
const authenticateToken = (req, res, next) => {
  const token = req.cookies.accessToken;

  if (!token) {
    return res.status(401).json({ error: 'Kein Token vorhanden' });
  }

  if (tokenBlacklist.has(token)) {
    return res.status(401).json({ error: 'Token wurde widerrufen' });
  }

  try {
    const user = jwt.verify(token, config.accessTokenSecret);
    req.user = user;
    next();
  } catch (err) {
    return res.status(403).json({ error: 'Ungültiger Token' });
  }
};

// CSRF Schutz Middleware
const validateCSRF = (req, res, next) => {
  const csrfToken = req.headers['x-xsrf-token'];
  const cookieToken = req.cookies['XSRF-TOKEN'];

  if (!csrfToken || !cookieToken || csrfToken !== cookieToken) {
    return res.status(403).json({ error: 'CSRF Token ungültig' });
  }
  next();
};

// Login Route
app.post('/api/login', async (req, res) => {
  const { username, password } = req.body;

  // Hier: Benutzer-Authentifizierung gegen Datenbank
  // ...

  // Beispiel-User für Demo
  const user = { id: 1, role: 'user' };

  const accessToken = generateAccessToken(user);
  const refreshToken = generateRefreshToken(user);

  // Sichere Cookie-Konfiguration
  res.cookie('accessToken', accessToken, {
    httpOnly: true,
    secure: true,
    sameSite: 'strict',
    maxAge: 15 * 60 * 1000 // 15 Minuten
  });

  res.cookie('refreshToken', refreshToken, {
    httpOnly: true,
    secure: true,
    sameSite: 'strict',
    path: '/api/refresh',
    maxAge: config.cookieMaxAge
  });

  res.json({ message: 'Login erfolgreich' });
});

// Token Refresh Route
app.post('/api/refresh', validateCSRF, async (req, res) => {
  const refreshToken = req.cookies.refreshToken;

  if (!refreshToken) {
    return res.status(401).json({ error: 'Kein Refresh Token' });
  }

  try {
    const user = jwt.verify(refreshToken, config.refreshTokenSecret);
    
    // Alten Access Token zur Blacklist hinzufügen
    const oldAccessToken = req.cookies.accessToken;
    if (oldAccessToken) {
      tokenBlacklist.add(oldAccessToken);
    }

    // Neue Tokens generieren (Token Rotation)
    const newAccessToken = generateAccessToken(user);
    const newRefreshToken = generateRefreshToken(user);

    res.cookie('accessToken', newAccessToken, {
      httpOnly: true,
      secure: true,
      sameSite: 'strict',
      maxAge: 15 * 60 * 1000
    });

    res.cookie('refreshToken', newRefreshToken, {
      httpOnly: true,
      secure: true,
      sameSite: 'strict',
      path: '/api/refresh',
      maxAge: config.cookieMaxAge
    });

    res.json({ message: 'Token erfolgreich erneuert' });
  } catch (err) {
    return res.status(403).json({ error: 'Ungültiger Refresh Token' });
  }
});

// Logout Route
app.post('/api/logout', validateCSRF, (req, res) => {
  const accessToken = req.cookies.accessToken;
  if (accessToken) {
    tokenBlacklist.add(accessToken);
  }

  res.clearCookie('accessToken');
  res.clearCookie('refreshToken');
  res.json({ message: 'Logout erfolgreich' });
});

// Geschützte Route Beispiel
app.get('/api/protected', authenticateToken, validateCSRF, (req, res) => {
  res.json({ message: 'Geschützter Endpunkt', user: req.user });
});

// Error Handler
app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).json({ error: 'Interner Server Fehler' });
});

module.exports = app;