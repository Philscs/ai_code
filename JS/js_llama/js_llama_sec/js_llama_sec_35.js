// security-system.js

const CSP = require('csp-bibliothek');
const CSS = require('css-bibliothek');
const csrfToken = require('csrf-token');
const ContextIsolation = require('context-isolation');
const Webhook = require('webhook-bibliothek');

// Content Security Policy
const cspRule = new CSP.Rule({
  'default-src': ['*'],
  'script-src': ['self', 'https://cdn.example.com'], // nur selbst und ein bestimmtes CDN 
erlauben
  'style-src': ['self', 'https://fonts.gstatic.com'], // nur selbst und Google Fonts 
erlauben
});

const csp = new CSP(cspRule);
csp.addPolicy({
  'script-src': ['*'],
  'style-src': ['*'], // weitere Regeln hinzufügen, wenn nötig
});

// XSS Prevention
const securityRule = new CSS.Rule({
  'script-src': ['*'],
});
CSS.addSecurityRule(securityRule);

// CSRF Protection
const token = csrfToken.generate();

function applyCsrfToken(request) {
  request.headers['X-CSRF-Token'] = token;
}

app.use(applyCsrfToken);

// Privilege Separation
const securityRule = new ContextIsolation.Rule({
  'worker-src': ['*'],
});
ContextIsolation.addSecurityRule(securityRule);

// Update Mechanism
const updateRule = new Webhook.Rule({
  'update-src': ['https://example.com/extension/update'],
});
Webhook.addSecurityRule(updateRule);

module.exports = {
  csp,
  securityRule,
  token,
};
