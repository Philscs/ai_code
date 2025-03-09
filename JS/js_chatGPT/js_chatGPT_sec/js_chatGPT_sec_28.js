// Import benötigter Module
const stackTrace = require('stack-trace');
const sourceMapSupport = require('source-map-support');
const crypto = require('crypto');

// Source Map Support aktivieren
sourceMapSupport.install();

// PII Detection Helper
function detectPII(message) {
  const piiPatterns = [
    /\b\d{3}-\d{2}-\d{4}\b/, // Beispiel: Sozialversicherungsnummer
    /\b\d{16}\b/, // Beispiel: Kreditkartennummer
    /\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b/ // Beispiel: E-Mail-Adresse
  ];
  return piiPatterns.some((pattern) => pattern.test(message));
}

// PII-Entfernung
function sanitizeData(message) {
  const piiPatterns = [
    { pattern: /\b\d{3}-\d{2}-\d{4}\b/, replacement: "[REDACTED-SSN]" },
    { pattern: /\b\d{16}\b/, replacement: "[REDACTED-CCN]" },
    { pattern: /\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b/, replacement: "[REDACTED-EMAIL]" }
  ];
  return piiPatterns.reduce((sanitized, { pattern, replacement }) => sanitized.replace(pattern, replacement), message);
}

// Rate Limiting
const errorCache = new Map();
function rateLimit(key, limit, interval) {
  const now = Date.now();
  if (!errorCache.has(key)) {
    errorCache.set(key, { count: 1, firstSeen: now });
    return true;
  }
  const data = errorCache.get(key);
  if (now - data.firstSeen > interval) {
    data.count = 1;
    data.firstSeen = now;
    return true;
  } else if (data.count < limit) {
    data.count += 1;
    return true;
  }
  return false;
}

// Error Tracking Hauptfunktion
function trackError(error) {
  const stack = stackTrace.parse(error);
  const message = error.message || "Unknown Error";
  const errorKey = crypto.createHash('sha256').update(message).digest('hex');

  if (!rateLimit(errorKey, 5, 60000)) {
    console.warn("Rate limit exceeded for error:", message);
    return;
  }

  const sanitizedMessage = sanitizeData(message);

  console.error("Tracked Error:", {
    message: sanitizedMessage,
    stack: stack.map((frame) => ({
      functionName: frame.getFunctionName(),
      fileName: frame.getFileName(),
      lineNumber: frame.getLineNumber(),
      columnNumber: frame.getColumnNumber()
    })),
    hasPII: detectPII(message),
    timestamp: new Date().toISOString()
  });
}

// Beispiel: Fehler auslösen und erfassen
try {
  throw new Error("Testfehler mit einer E-Mail: test@example.com");
} catch (error) {
  trackError(error);
}
