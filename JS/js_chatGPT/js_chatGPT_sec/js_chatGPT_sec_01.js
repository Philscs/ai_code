const express = require('express');
const jwt = require('jsonwebtoken');
const cookieParser = require('cookie-parser');
const bcrypt = require('bcrypt');
const crypto = require('crypto');
const { v4: uuidv4 } = require('uuid');

const app = express();
const PORT = 3000;

app.use(express.json());
app.use(cookieParser());

const JWT_SECRET = 'your_jwt_secret';
const JWT_REFRESH_SECRET = 'your_refresh_secret';
const ACCESS_TOKEN_LIFETIME = '15m';
const REFRESH_TOKEN_LIFETIME = '7d';

let users = []; // In-Memory user store (for demo purposes)
let refreshTokenStore = []; // In-Memory refresh token store

// Middleware: Verify Access Token
function authenticateToken(req, res, next) {
    const token = req.cookies['access_token'];
    if (!token) return res.status(401).json({ message: 'Unauthorized' });

    jwt.verify(token, JWT_SECRET, (err, user) => {
        if (err) return res.status(403).json({ message: 'Forbidden' });
        req.user = user;
        next();
    });
}

// Utility: Generate Access Token
function generateAccessToken(user) {
    return jwt.sign({ id: user.id, email: user.email }, JWT_SECRET, { expiresIn: ACCESS_TOKEN_LIFETIME });
}

// Utility: Generate Refresh Token
function generateRefreshToken(user) {
    const refreshToken = jwt.sign({ id: user.id }, JWT_REFRESH_SECRET, { expiresIn: REFRESH_TOKEN_LIFETIME });
    refreshTokenStore.push(refreshToken); // Store refresh token
    return refreshToken;
}

// Register Route
app.post('/register', async (req, res) => {
    const { email, password } = req.body;
    if (!email || !password) return res.status(400).json({ message: 'Email and Password required' });

    const hashedPassword = await bcrypt.hash(password, 10);
    const newUser = { id: uuidv4(), email, password: hashedPassword };
    users.push(newUser);

    res.status(201).json({ message: 'User registered successfully' });
});

// Login Route
app.post('/login', async (req, res) => {
    const { email, password } = req.body;
    const user = users.find(u => u.email === email);
    if (!user || !(await bcrypt.compare(password, user.password))) {
        return res.status(403).json({ message: 'Invalid credentials' });
    }

    const accessToken = generateAccessToken(user);
    const refreshToken = generateRefreshToken(user);

    // Secure cookie configuration
    res.cookie('access_token', accessToken, {
        httpOnly: true,
        secure: true,
        sameSite: 'Strict',
        maxAge: 15 * 60 * 1000 // 15 minutes
    });
    res.cookie('refresh_token', refreshToken, {
        httpOnly: true,
        secure: true,
        sameSite: 'Strict',
        maxAge: 7 * 24 * 60 * 60 * 1000 // 7 days
    });

    res.json({ message: 'Login successful' });
});

// Token Refresh Route
app.post('/token', (req, res) => {
    const refreshToken = req.cookies['refresh_token'];
    if (!refreshToken || !refreshTokenStore.includes(refreshToken)) {
        return res.status(403).json({ message: 'Invalid refresh token' });
    }

    jwt.verify(refreshToken, JWT_REFRESH_SECRET, (err, user) => {
        if (err) return res.status(403).json({ message: 'Invalid refresh token' });

        // Token rotation: invalidate old refresh token
        refreshTokenStore = refreshTokenStore.filter(rt => rt !== refreshToken);
        const newAccessToken = generateAccessToken(user);
        const newRefreshToken = generateRefreshToken(user);

        res.cookie('access_token', newAccessToken, {
            httpOnly: true,
            secure: true,
            sameSite: 'Strict',
            maxAge: 15 * 60 * 1000
        });
        res.cookie('refresh_token', newRefreshToken, {
            httpOnly: true,
            secure: true,
            sameSite: 'Strict',
            maxAge: 7 * 24 * 60 * 60 * 1000
        });

        res.json({ message: 'Token refreshed' });
    });
});

// Logout Route
app.post('/logout', (req, res) => {
    const refreshToken = req.cookies['refresh_token'];
    if (refreshToken) {
        refreshTokenStore = refreshTokenStore.filter(rt => rt !== refreshToken);
    }

    res.clearCookie('access_token');
    res.clearCookie('refresh_token');

    res.json({ message: 'Logged out successfully' });
});

// Protected Route
app.get('/protected', authenticateToken, (req, res) => {
    res.json({ message: `Welcome, user ${req.user.email}` });
});

app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});
