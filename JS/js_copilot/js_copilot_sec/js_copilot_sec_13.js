const fs = require('fs');
const JSONStream = require('JSONStream');
const Ajv = require('ajv');
const os = require('os');

// Schema Validation Setup
const ajv = new Ajv();
const schema = {
    type: 'object',
    properties: {
        id: { type: 'number' },
        name: { type: 'string' },
        active: { type: 'boolean' },
    },
    required: ['id', 'name'],
};
const validate = ajv.compile(schema);

class StreamParser {
    constructor(filePath, options = {}) {
        this.filePath = filePath;
        this.chunkSizeLimit = options.chunkSizeLimit || 10 * 1024 * 1024; // 10 MB
        this.maxMemoryUsage = options.maxMemoryUsage || os.totalmem() * 0.5; // 50% of system memory
        this.errorRecovery = options.errorRecovery || true;
        this.processedChunks = 0;
        this.validEntries = 0;
        this.invalidEntries = 0;
    }

    monitorMemory() {
        const usedMemory = process.memoryUsage().heapUsed;
        if (usedMemory > this.maxMemoryUsage) {
            throw new Error(`Memory usage exceeded: ${usedMemory} bytes used.`);
        }
    }

    parse() {
        const stream = fs.createReadStream(this.filePath, { encoding: 'utf8' });
        const jsonStream = JSONStream.parse('*');

        stream.on('error', (err) => {
            console.error('Error reading file:', err.message);
            if (!this.errorRecovery) throw err;
        });

        jsonStream.on('data', (data) => {
            try {
                this.monitorMemory();

                if (validate(data)) {
                    console.log('Valid entry:', data);
                    this.validEntries++;
                } else {
                    console.warn('Invalid entry:', data);
                    this.invalidEntries++;
                }

                this.processedChunks++;
                if (this.processedChunks % 1000 === 0) {
                    console.log(`${this.processedChunks} chunks processed.`);
                }
            } catch (err) {
                console.error('Processing error:', err.message);
                if (!this.errorRecovery) throw err;
            }
        });

        jsonStream.on('end', () => {
            console.log('Parsing completed.');
            console.log(`Valid entries: ${this.validEntries}, Invalid entries: ${this.invalidEntries}`);
        });

        jsonStream.on('error', (err) => {
            console.error('Error in JSON parsing:', err.message);
            if (!this.errorRecovery) throw err;
        });

        stream.pipe(jsonStream);
    }
}

// Usage example
const parser = new StreamParser('large-file.json', {
    chunkSizeLimit: 5 * 1024 * 1024, // 5 MB
    maxMemoryUsage: os.totalmem() * 0.2, // 20% of system memory
    errorRecovery: true,
});

parser.parse();
