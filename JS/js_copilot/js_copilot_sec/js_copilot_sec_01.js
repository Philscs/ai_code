
const express = require('express');
const jwt = require('jsonwebtoken');
const cookieParser = require('cookie-parser');
const csrf = require('csurf');
const helmet = require('helmet');

const app = express();
app.use(express.json());
app.use(cookieParser());
app.use(csrf({ cookie: true }));
app.use(helmet());

// Set up JWT secret key
const jwtSecret = 'your-secret-key';

// In-memory storage for refresh tokens
const refreshTokens = [];

// Generate access token
function generateAccessToken(user) {
  return jwt.sign(user, jwtSecret, { expiresIn: '15m' });
}

// Generate refresh token
function generateRefreshToken(user) {
  const refreshToken = jwt.sign(user, jwtSecret);
  refreshTokens.push(refreshToken);
  return refreshToken;
}

// Verify access token
function verifyAccessToken(req, res, next) {
  const token = req.cookies.accessToken;
  if (!token) return res.sendStatus(401);

  jwt.verify(token, jwtSecret, (err, user) => {
    if (err) return res.sendStatus(403);
    req.user = user;
    next();
  });
}

// Verify CSRF token
function verifyCSRFToken(req, res, next) {
  if (req.csrfToken() !== req.headers['x-csrf-token']) {
    return res.sendStatus(403);
  }
  next();
}

// Login route
app.post('/login', (req, res) => {
  // Authenticate user
  const user = { id: 1, username: 'example' };

  // Generate access token and refresh token
  const accessToken = generateAccessToken(user);
  const refreshToken = generateRefreshToken(user);

  // Set access token as a secure HttpOnly cookie
  res.cookie('accessToken', accessToken, {
    httpOnly: true,
    secure: true,
    sameSite: 'strict',
  });

  // Return refresh token as a response
  res.json({ refreshToken });
});

// Refresh token route
app.post('/refresh-token', (req, res) => {
  const { refreshToken } = req.body;
  if (!refreshToken || !refreshTokens.includes(refreshToken)) {
    return res.sendStatus(401);
  }

  jwt.verify(refreshToken, jwtSecret, (err, user) => {
    if (err) return res.sendStatus(403);

    // Generate new access token
    const accessToken = generateAccessToken(user);

    // Set new access token as a secure HttpOnly cookie
    res.cookie('accessToken', accessToken, {
      httpOnly: true,
      secure: true,
      sameSite: 'strict',
    });

    res.sendStatus(200);
  });
});

// Logout route
app.post('/logout', (req, res) => {
  const { refreshToken } = req.body;
  if (refreshToken) {
    // Remove refresh token from the list
    const index = refreshTokens.indexOf(refreshToken);
    if (index !== -1) {
      refreshTokens.splice(index, 1);
    }
  }

  // Clear access token cookie
  res.clearCookie('accessToken');

  res.sendStatus(200);
});

// Protected route
app.get('/protected', verifyAccessToken, verifyCSRFToken, (req, res) => {
  res.json({ message: 'Protected route accessed successfully' });
});

// Start the server
app.listen(3000, () => {
  console.log('Server started on port 3000');
});
