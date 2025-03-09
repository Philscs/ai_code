const fs = require("fs");
const path = require("path");

class LogAggregator {
  constructor(options) {
    this.logFilePath = options.logFilePath || "logs.json";
    this.maxFileSize = options.maxFileSize || 1024 * 1024; // 1 MB
    this.logLevels = ["DEBUG", "INFO", "WARN", "ERROR"];
    this.alertTriggers = options.alertTriggers || [];
    this.initializeLogFile();
  }

  initializeLogFile() {
    if (!fs.existsSync(this.logFilePath)) {
      fs.writeFileSync(this.logFilePath, JSON.stringify([]));
    }
  }

  log(level, message, metadata = {}) {
    if (!this.logLevels.includes(level)) {
      throw new Error(`Invalid log level: ${level}`);
    }

    const logEntry = {
      timestamp: new Date().toISOString(),
      level,
      message,
      metadata,
    };

    this.writeLog(logEntry);
    this.checkTriggers(logEntry);
  }

  writeLog(logEntry) {
    const logs = this.readLogs();
    logs.push(logEntry);
    fs.writeFileSync(this.logFilePath, JSON.stringify(logs, null, 2));

    if (fs.statSync(this.logFilePath).size > this.maxFileSize) {
      this.rotateLogs();
    }
  }

  rotateLogs() {
    const archivePath = `${this.logFilePath}.${Date.now()}`;
    fs.renameSync(this.logFilePath, archivePath);
    this.initializeLogFile();
  }

  readLogs() {
    return JSON.parse(fs.readFileSync(this.logFilePath));
  }

  search(criteria) {
    const logs = this.readLogs();
    return logs.filter((log) => {
      return Object.keys(criteria).every((key) => log[key] === criteria[key]);
    });
  }

  checkTriggers(logEntry) {
    for (const trigger of this.alertTriggers) {
      if (trigger.condition(logEntry)) {
        trigger.action(logEntry);
      }
    }
  }
}

// Example usage
const logAggregator = new LogAggregator({
  logFilePath: path.join(__dirname, "logs.json"),
  maxFileSize: 1024 * 10, // 10 KB for testing
  alertTriggers: [
    {
      condition: (log) => log.level === "ERROR",
      action: (log) => console.error("ALERT:", log),
    },
  ],
});

logAggregator.log("INFO", "This is an info log", { userId: 123 });
logAggregator.log("ERROR", "This is an error log", { userId: 456 });

console.log("Search Results:", logAggregator.search({ level: "ERROR" }));
