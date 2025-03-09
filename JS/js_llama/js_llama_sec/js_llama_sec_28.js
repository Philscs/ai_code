const errors = require('errors');
const consoleError = require('console-error');
const sourceMapSupport = require('source-map-support').install;
const piiDetection = require('pii-detection');

// Erstelle ein Error Tracking System
class ErrorTracker {
  constructor(options) {
    this.options = options || {};
    this.errors = [];
    this.sourceMapUrl = this.options.sourceMapUrl || 'https://example.com/source-map';
    this.rateLimit = this.options.rateLimit || 100;
    this.piiDetection = piiDetection();
    sourceMapSupport.load(sourceMapUrl);
  }

  // Verarbeitet eine Fehlermeldung und fügt sie dem Array hinzu
  trackError(error) {
    if (!error.message) return;

    const errorMessage = `Error: ${error.message}`;
    console.error(errorMessage);

    // Verarbeitet den Stack Trace
    const stackTrace = this.processStackTrace(error.stack);
    this.errors.push({ message: errorMessage, stackTrace });

    // Überprüft ob der Fehler eine PII enthält
    if (this.piiDetection.detect(errorMessage)) {
      console.warn('Pernicious Information Detected');
    }

    // Verarbeitet die Quellkarte
    const sourceMap = this.processSourceMap(error.stack);
    this.errors.push({ message: errorMessage, sourceMap });

    // Implementiert die Rate Limiting
    if (this.errors.length >= this.rateLimit) {
      console.warn(`Rate limit reached. ${this.rateLimit} errors in the last minute.`);
    }
  }

  // Verarbeitet den Stack Trace
  processStackTrace(stackTrace) {
    const lines = stackTrace.split('\n');
    return lines.map(line => line.trim());
  }

  // Verarbeitet die Quellkarte
  processSourceMap(stackTrace) {
    const sourceMapUrl = this.sourceMapUrl;
    const lineNumbers = [];
    for (const line of stackTrace) {
      if (!line.includes('at')) continue;
      const match = line.match(/at (.*) \((\d+):(\d+)\)/);
      if (!match) continue;
      const functionName = match[1];
      const lineNumber = parseInt(match[2], 10) - 1;
      lineNumbers.push({ functionName, lineNumber });
    }
    return sourceMapUrl + '?lines=1%2C' + lineNumbers.map(lineNumber => 
`${lineNumber+1}:${lineNumber}`).join('&');
  }

  // Sanisiert Daten
  sanitizeData(data) {
    const sanitizedData = {};
    for (const key in data) {
      if (typeof data[key] === 'string') {
        sanitizedData[key] = data[key].trim();
      } else {
        sanitizedData[key] = data[key];
      }
    }
    return sanitizedData;
  }

  // Exportiere die Error-Tracker-Instanz
  export() {
    return this;
  }
}

// Verwende den Error Tracker
const errorTracker = new ErrorTracker();

errorTracker.trackError({
  name: 'John Doe',
  age: 30,
  occupation: 'Software Engineer'
});

console.log(errorTracker.errors);