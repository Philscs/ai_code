// Log Levels Definition
const LogLevel = {
    DEBUG: 0,
    INFO: 1,
    WARN: 2,
    ERROR: 3,
    FATAL: 4,
    labels: ['DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL']
};

class LogEntry {
    constructor(level, message, metadata = {}) {
        this.timestamp = new Date().toISOString();
        this.level = level;
        this.message = message;
        this.metadata = metadata;
    }

    toString() {
        return JSON.stringify({
            timestamp: this.timestamp,
            level: LogLevel.labels[this.level],
            message: this.message,
            metadata: this.metadata
        });
    }
}

class LogRotator {
    constructor(maxFileSize = 5 * 1024 * 1024, maxFiles = 5) { // 5MB default
        this.maxFileSize = maxFileSize;
        this.maxFiles = maxFiles;
        this.currentFileSize = 0;
        this.currentFileIndex = 0;
        this.logs = new Map(); // Simulating files in memory
    }

    write(logEntry) {
        const logString = logEntry.toString() + '\n';
        const logSize = new TextEncoder().encode(logString).length;

        if (this.currentFileSize + logSize > this.maxFileSize) {
            this.rotate();
        }

        const currentLogs = this.logs.get(this.currentFileIndex) || '';
        this.logs.set(this.currentFileIndex, currentLogs + logString);
        this.currentFileSize += logSize;
    }

    rotate() {
        this.currentFileIndex = (this.currentFileIndex + 1) % this.maxFiles;
        this.currentFileSize = 0;
        
        // Clear oldest log file if it exists
        this.logs.delete(this.currentFileIndex);
    }

    getLogs(fileIndex) {
        return this.logs.get(fileIndex) || '';
    }
}

class AlertTrigger {
    constructor(condition, action) {
        this.condition = condition;
        this.action = action;
    }

    evaluate(logEntry) {
        if (this.condition(logEntry)) {
            this.action(logEntry);
        }
    }
}

class LogAggregator {
    constructor(options = {}) {
        this.minimumLevel = options.minimumLevel || LogLevel.DEBUG;
        this.rotator = new LogRotator(options.maxFileSize, options.maxFiles);
        this.alerts = new Set();
        this.searchIndex = new Map(); // Simple in-memory search index
    }

    log(level, message, metadata = {}) {
        if (level < this.minimumLevel) return;

        const entry = new LogEntry(level, message, metadata);
        
        // Write to log file
        this.rotator.write(entry);
        
        // Index for search
        this.indexLog(entry);
        
        // Check alerts
        this.checkAlerts(entry);
    }

    // Convenience methods for different log levels
    debug(message, metadata) { this.log(LogLevel.DEBUG, message, metadata); }
    info(message, metadata) { this.log(LogLevel.INFO, message, metadata); }
    warn(message, metadata) { this.log(LogLevel.WARN, message, metadata); }
    error(message, metadata) { this.log(LogLevel.ERROR, message, metadata); }
    fatal(message, metadata) { this.log(LogLevel.FATAL, message, metadata); }

    addAlert(condition, action) {
        const trigger = new AlertTrigger(condition, action);
        this.alerts.add(trigger);
        return trigger;
    }

    removeAlert(trigger) {
        this.alerts.delete(trigger);
    }

    checkAlerts(logEntry) {
        for (const trigger of this.alerts) {
            trigger.evaluate(logEntry);
        }
    }

    indexLog(logEntry) {
        // Create searchable tokens from the log entry
        const tokens = new Set([
            ...logEntry.message.toLowerCase().split(/\W+/),
            LogLevel.labels[logEntry.level].toLowerCase(),
            ...Object.values(logEntry.metadata).map(v => String(v).toLowerCase())
        ]);

        // Index each token
        for (const token of tokens) {
            if (!token) continue;
            
            if (!this.searchIndex.has(token)) {
                this.searchIndex.set(token, []);
            }
            this.searchIndex.get(token).push(logEntry);
        }
    }

    search(query, options = {}) {
        const searchTokens = query.toLowerCase().split(/\W+/).filter(Boolean);
        const results = new Map();

        for (const token of searchTokens) {
            const matches = this.searchIndex.get(token) || [];
            
            for (const entry of matches) {
                const count = (results.get(entry) || 0) + 1;
                results.set(entry, count);
            }
        }

        // Filter and sort results
        return Array.from(results.entries())
            .filter(([entry, matchCount]) => {
                if (options.level !== undefined && entry.level !== options.level) return false;
                if (options.startTime && new Date(entry.timestamp) < options.startTime) return false;
                if (options.endTime && new Date(entry.timestamp) > options.endTime) return false;
                return matchCount === searchTokens.length; // All tokens must match
            })
            .map(([entry]) => entry)
            .sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));
    }
}

// Example usage:
const logger = new LogAggregator({
    minimumLevel: LogLevel.DEBUG,
    maxFileSize: 1024 * 1024, // 1MB
    maxFiles: 5
});

// Add an alert for errors
logger.addAlert(
    (entry) => entry.level >= LogLevel.ERROR,
    (entry) => console.error(`Alert! Critical error occurred: ${entry.message}`)
);

// Log some events
logger.info("User login", { userId: "123", ip: "192.168.1.1" });
logger.error("Database connection failed", { 
    dbHost: "db.example.com",
    errorCode: "ECONNREFUSED"
});

// Search logs
const results = logger.search("database connection", {
    level: LogLevel.ERROR,
    startTime: new Date(Date.now() - 24 * 60 * 60 * 1000) // Last 24 hours
});

// Export for use in Node.js or browser
export default LogAggregator;
export { LogLevel, LogEntry, LogRotator, AlertTrigger };