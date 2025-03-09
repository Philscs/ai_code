// Error Tracking System

// Rate Limiter Implementation
class RateLimiter {
    constructor(maxRequests, timeWindow) {
        this.maxRequests = maxRequests;
        this.timeWindow = timeWindow;
        this.requests = new Map();
    }

    isAllowed(identifier) {
        const now = Date.now();
        const windowStart = now - this.timeWindow;

        // Clean up old requests
        this.requests.forEach((timestamp, key) => {
            if (timestamp < windowStart) {
                this.requests.delete(key);
            }
        });

        // Get requests count for this identifier
        const requestCount = [...this.requests.values()].filter(
            timestamp => timestamp >= windowStart
        ).length;

        if (requestCount >= this.maxRequests) {
            return false;
        }

        this.requests.set(identifier + now, now);
        return true;
    }
}

// PII Detection
class PIIDetector {
    constructor() {
        this.patterns = {
            email: /[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}/g,
            creditCard: /\b\d{4}[- ]?\d{4}[- ]?\d{4}[- ]?\d{4}\b/g,
            phone: /\b\+?[\d()-]{10,}\b/g,
            ssn: /\b\d{3}-\d{2}-\d{4}\b/g
        };
    }

    detect(text) {
        const findings = {};
        for (const [type, pattern] of Object.entries(this.patterns)) {
            const matches = text.match(pattern);
            if (matches) {
                findings[type] = matches;
            }
        }
        return findings;
    }

    redact(text) {
        let redactedText = text;
        for (const pattern of Object.values(this.patterns)) {
            redactedText = redactedText.replace(pattern, '[REDACTED]');
        }
        return redactedText;
    }
}

// Data Sanitizer
class DataSanitizer {
    static sanitize(data) {
        if (typeof data === 'string') {
            return this.sanitizeString(data);
        } else if (Array.isArray(data)) {
            return data.map(item => this.sanitize(item));
        } else if (data && typeof data === 'object') {
            return this.sanitizeObject(data);
        }
        return data;
    }

    static sanitizeString(str) {
        return str
            .replace(/[<>]/g, '')  // Remove potential HTML/XML tags
            .replace(/javascript:/gi, '')  // Remove JavaScript protocol
            .replace(/on\w+=/gi, '')  // Remove event handlers
            .trim();
    }

    static sanitizeObject(obj) {
        const sanitized = {};
        for (const [key, value] of Object.entries(obj)) {
            sanitized[key] = this.sanitize(value);
        }
        return sanitized;
    }
}

// Stack Trace Processor
class StackTraceProcessor {
    constructor(sourceMapSupport = false) {
        this.sourceMapSupport = sourceMapSupport;
        this.sourceMapConsumer = null;
    }

    async processError(error) {
        const stackLines = error.stack.split('\n');
        const processedStack = [];

        for (let line of stackLines) {
            // Basic stack line parsing
            const match = line.match(/at\s+(.+?)\s+\((.+):(\d+):(\d+)\)/);
            if (!match) continue;

            const [, functionName, fileName, lineNumber, columnNumber] = match;
            
            let location = {
                functionName,
                fileName,
                lineNumber: parseInt(lineNumber),
                columnNumber: parseInt(columnNumber)
            };

            // Apply source map transformation if enabled
            if (this.sourceMapSupport && this.sourceMapConsumer) {
                try {
                    const originalPosition = await this.sourceMapConsumer.originalPositionFor({
                        line: location.lineNumber,
                        column: location.columnNumber
                    });
                    
                    if (originalPosition.source) {
                        location = {
                            functionName: originalPosition.name || location.functionName,
                            fileName: originalPosition.source,
                            lineNumber: originalPosition.line,
                            columnNumber: originalPosition.column
                        };
                    }
                } catch (e) {
                    console.warn('Source map transformation failed:', e);
                }
            }

            processedStack.push(location);
        }

        return processedStack;
    }

    async loadSourceMap(sourceMapData) {
        if (this.sourceMapSupport) {
            // In a real implementation, you would use a source map library
            // this.sourceMapConsumer = await new SourceMapConsumer(sourceMapData);
            console.log('Source map loaded');
        }
    }
}

// Main Error Tracker
class ErrorTracker {
    constructor(config = {}) {
        this.rateLimiter = new RateLimiter(
            config.maxRequests || 100,
            config.timeWindow || 60000
        );
        this.piiDetector = new PIIDetector();
        this.stackTraceProcessor = new StackTraceProcessor(config.sourceMapSupport);
        this.errorStore = new Set();
    }

    async trackError(error, context = {}) {
        // Check rate limit
        if (!this.rateLimiter.isAllowed(error.message)) {
            console.warn('Rate limit exceeded for error tracking');
            return;
        }

        try {
            // Process stack trace
            const processedStack = await this.stackTraceProcessor.processError(error);

            // Sanitize context data
            const sanitizedContext = DataSanitizer.sanitize(context);

            // Detect and redact PII
            const errorMessage = this.piiDetector.redact(error.message);
            const contextString = JSON.stringify(sanitizedContext);
            const piiFindings = this.piiDetector.detect(contextString);

            // Prepare error report
            const errorReport = {
                timestamp: new Date().toISOString(),
                message: errorMessage,
                stack: processedStack,
                context: sanitizedContext,
                piiDetected: Object.keys(piiFindings).length > 0,
            };

            // Store error report
            this.errorStore.add(errorReport);

            return errorReport;
        } catch (e) {
            console.error('Error processing error:', e);
            throw e;
        }
    }

    getStoredErrors() {
        return [...this.errorStore];
    }

    clearErrors() {
        this.errorStore.clear();
    }
}

// Usage Example
const errorTracker = new ErrorTracker({
    maxRequests: 100,
    timeWindow: 60000,
    sourceMapSupport: true
});

// Example error tracking
try {
    throw new Error('Something went wrong!');
} catch (error) {
    errorTracker.trackError(error, {
        userId: '12345',
        email: 'user@example.com',
        additionalInfo: 'Some context'
    }).then(report => {
        console.log('Error tracked:', report);
    }).catch(err => {
        console.error('Failed to track error:', err);
    });
}