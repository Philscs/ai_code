const { Transform } = require('stream');

class JSONStreamParser extends Transform {
    constructor(options = {}) {
        super({ objectMode: true });
        
        // Parser state
        this.buffer = '';
        this.depth = 0;
        this.isInString = false;
        this.isEscaped = false;
        
        // Configuration
        this.maxBufferSize = options.maxBufferSize || 10 * 1024 * 1024; // 10MB default
        this.maxDepth = options.maxDepth || 100;
        this.schema = options.schema || null;
        
        // Performance monitoring
        this.bytesProcessed = 0;
        this.startTime = Date.now();
        this.lastMemoryUsage = process.memoryUsage();
        
        // Error recovery
        this.errorCount = 0;
        this.maxErrors = options.maxErrors || 3;
    }

    _transform(chunk, encoding, callback) {
        try {
            // DOS Protection: Check processing rate
            this.bytesProcessed += chunk.length;
            const elapsedSeconds = (Date.now() - this.startTime) / 1000;
            const processingRate = this.bytesProcessed / elapsedSeconds;
            
            if (processingRate > this.maxBufferSize) {
                throw new Error('Processing rate exceeded safety threshold');
            }

            // Memory monitoring
            const currentMemory = process.memoryUsage();
            const memoryDelta = currentMemory.heapUsed - this.lastMemoryUsage.heapUsed;
            
            if (memoryDelta > this.maxBufferSize) {
                this.emit('memoryWarning', {
                    delta: memoryDelta,
                    current: currentMemory.heapUsed
                });
            }
            
            this.lastMemoryUsage = currentMemory;

            // Process the chunk
            this.buffer += chunk.toString();
            this._processBuffer();
            
            callback();
        } catch (error) {
            this._handleError(error, callback);
        }
    }

    _processBuffer() {
        let startPos = 0;
        
        while (startPos < this.buffer.length) {
            // Find the next complete JSON object
            const result = this._findCompleteObject(startPos);
            
            if (!result) break;
            
            const { start, end } = result;
            
            try {
                const jsonStr = this.buffer.slice(start, end);
                const parsed = JSON.parse(jsonStr);
                
                // Schema validation
                if (this.schema && !this._validateSchema(parsed)) {
                    throw new Error('Schema validation failed');
                }
                
                this.push(parsed);
                startPos = end;
            } catch (error) {
                this._handleError(error);
                startPos = end;
            }
        }
        
        // Trim processed data from buffer
        this.buffer = this.buffer.slice(startPos);
        
        // Buffer size protection
        if (this.buffer.length > this.maxBufferSize) {
            throw new Error('Buffer size limit exceeded');
        }
    }

    _findCompleteObject(startPos) {
        let depth = 0;
        let inString = false;
        let escaped = false;
        
        for (let i = startPos; i < this.buffer.length; i++) {
            const char = this.buffer[i];
            
            if (!inString) {
                if (char === '{') {
                    depth++;
                    if (depth > this.maxDepth) {
                        throw new Error('Maximum depth exceeded');
                    }
                } else if (char === '}') {
                    depth--;
                    if (depth === 0) {
                        return { start: startPos, end: i + 1 };
                    }
                }
            }
            
            if (char === '"' && !escaped) {
                inString = !inString;
            }
            
            escaped = char === '\\' && !escaped;
        }
        
        return null;
    }

    _validateSchema(data) {
        if (!this.schema) return true;
        
        try {
            // Implement schema validation logic here
            // This is a simple example - replace with your schema validation library
            const validateProperty = (schema, value) => {
                if (schema.type && typeof value !== schema.type) {
                    return false;
                }
                
                if (schema.properties) {
                    for (const [key, propSchema] of Object.entries(schema.properties)) {
                        if (!validateProperty(propSchema, value[key])) {
                            return false;
                        }
                    }
                }
                
                return true;
            };
            
            return validateProperty(this.schema, data);
        } catch (error) {
            this.emit('validationError', error);
            return false;
        }
    }

    _handleError(error, callback) {
        this.errorCount++;
        this.emit('parseError', error);
        
        if (this.errorCount > this.maxErrors) {
            callback(error);
            return;
        }
        
        // Try to recover by finding the next valid JSON start
        const nextStart = this.buffer.indexOf('{', 1);
        if (nextStart !== -1) {
            this.buffer = this.buffer.slice(nextStart);
        } else {
            this.buffer = '';
        }
        
        if (callback) callback();
    }
}

// Usage example
module.exports = function createJSONStreamParser(options = {}) {
    return new JSONStreamParser(options);
};
