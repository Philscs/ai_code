const fs = require('fs');
const path = require('path');

// Log Levels
const LOG_LEVELS = {
  DEBUG: 'DEBUG',
  INFO: 'INFO',
  WARNING: 'WARNING',
  ERROR: 'ERROR',
  CRITICAL: 'CRITICAL'
};

// Structured Logging
class LogEntry {
  constructor(level, message, metadata) {
    this.level = level;
    this.message = message;
    this.metadata = metadata || {};
  }
}

class Logger {
  constructor(logFile) {
    this.logFile = logFile;
    this.logEntries = [];
  }

  debug(message) {
    const entry = new LogEntry(LOG_LEVELS.DEBUG, message);
    this.logEntries.push(entry);
    this.rotateLog();
  }

  info(message) {
    const entry = new LogEntry(LOG_LEVELS.INFO, message);
    this.logEntries.push(entry);
    this.rotateLog();
  }

  warning(message) {
    const entry = new LogEntry(LOG_LEVELS.WARNING, message);
    this.logEntries.push(entry);
    this.rotateLog();
  }

  error(message) {
    const entry = new LogEntry(LOG_LEVELS.ERROR, message);
    this.logEntries.push(entry);
    this.rotateLog();
  }

  critical(message) {
    const entry = new LogEntry(LOG_LEVELS.CRITICAL, message);
    this.logEntries.push(entry);
    this.rotateLog();
  }

  search(query) {
    return this.logEntries.filter(entry => entry.message.includes(query));
  }
}

class LogAggregator {
  constructor(logFile) {
    this.logger = new Logger(logFile);
  }

  setLogLevel(level) {
    // implemente die Änderung des Log Levels
  }

  rotateLog() {
    const logDirectory = path.dirname(this.logger.logFile);
    if (!fs.existsSync(logDirectory)) {
      fs.mkdirSync(logDirectory, { recursive: true });
    }
    const currentDate = new Date();
    const logFileName = `log-${currentDate.toISOString().replace(/-/g, '')}.txt`;
    this.logger.logFile = path.join(logDirectory, logFileName);
  }

  setAlertTrigger(query) {
    // implemente die Alarmfunktion
  }
}

// Beispielnutzung
const aggregator = new LogAggregator('log.txt');

aggregator.debug('Ein Debug-Nachricht');
aggregator.info('Eine Info-Nachricht');
aggregator.warning('Eine Warnung Nachricht');
aggregator.error('Eine Fehlermeldung');
aggregator.critical('Eine kritische Meldung');

console.log(aggregator.search('Nachricht'));

// Alarm auslösen
aggregator.setAlertTrigger('Fehlermeldung').then(() => {
  console.log('Alarm ausgelöst!');
});