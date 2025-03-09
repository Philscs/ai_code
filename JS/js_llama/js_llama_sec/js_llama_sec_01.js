const express = require('express');
const jwt = require('jsonwebtoken');
const bcrypt = require('bcryptjs');
const helmet = require('helmet');
const passport = require('passport');

const app = express();
app.use(helmet());
app.use(express.json());

// Erstellung der Datenbank
const users = [
  {
    username: 'benutzer1',
    password: bcrypt.hashSync('passwort1', 10),
  },
];

// Funktion zum Verifizieren eines JWT-Tokens
function authenticate(req, res, next) {
  const token = req.header('Authorization');

  if (!token) {
    return res.status(401).send('Un autorisiert');
  }

  jwt.verify(token, 'your-secret-key', (err, decodedToken) => {
    if (err || !decodedToken) {
      return res.status(401).send('Falsches JWT-Token');
    }

    req.user = decodedToken.username;
    next();
  });
}

// Funktion zum Generieren eines neuen JWT-Tokens
function generateJwtToken(user) {
  const newToken = jwt.sign({ username: user.username }, 'your-secret-key', { expiresIn: '1h' });

  return newToken;
}

// Funktion zum Erstellen eines Refresh Tokens
function generateRefreshToken(user) {
  const refreshToken = jwt.sign({ username: user.username, iat: Date.now() }, 
'your-refresh-secret-key');

  return refreshToken;
}

// Route für die Anmeldung
app.post('/login', (req, res) => {
  const { username, password } = req.body;

  // Verifizierung der Benutzerdaten
  const user = users.find((user) => user.username === username);

  if (!user || !bcrypt.compareSync(password, user.password)) {
    return res.status(401).send('Falsche Anmeldedaten');
  }

  // Erstellung eines neuen JWT-Tokens
  const newToken = generateJwtToken(user);
  const refreshToken = generateRefreshToken(user);

  res.json({ token: newToken, refreshToken });
});

// Route für die Beuthung des Refresh Tokens
app.post('/refresh', authenticate, (req, res) => {
  const { refreshToken } = req.body;

  jwt.verify(refreshToken, 'your-refresh-secret-key', (err, decodedToken) => {
    if (err || !decodedToken) {
      return res.status(401).send('Falsches Refresh-Token');
    }

    // Erstellung eines neuen JWT-Tokens
    const newToken = generateJwtToken(decodedToken.username);
    res.json({ token: newToken });
  });
});

// Route für die Beuthung des JWT-Tokens
app.post('/protected', authenticate, (req, res) => {
  res.send('Berechtigt');
});

// Starte der Server
const port = 3000;
app.listen(port, () => {
  console.log(`Server läuft auf Port ${port}`);
});