// Dependencies
const Redis = require('ioredis');
const crypto = require('crypto');
const winston = require('winston');

class PubSubSystem {
    constructor(config = {}) {
        this.publisher = new Redis(config.redis);
        this.subscriber = new Redis(config.redis);
        this.retryDelays = config.retryDelays || [1000, 5000, 15000, 30000]; // Retry delays in ms
        this.maxRetries = config.maxRetries || 3;
        
        // Initialize logger
        this.logger = winston.createLogger({
            level: 'info',
            format: winston.format.json(),
            transports: [
                new winston.transports.File({ filename: 'error.log', level: 'error' }),
                new winston.transports.File({ filename: 'combined.log' })
            ]
        });

        // Metrics for monitoring
        this.metrics = {
            messagesPublished: 0,
            messagesConsumed: 0,
            messagesFailed: 0,
            messagesRetried: 0,
            deadLetters: 0
        };

        // Security settings
        this.encryptionKey = config.encryptionKey;
        this.accessControlList = config.accessControlList || {};
    }

    // Encrypt message if encryption is enabled
    encrypt(message) {
        if (!this.encryptionKey) return message;
        
        const iv = crypto.randomBytes(16);
        const cipher = crypto.createCipheriv('aes-256-gcm', Buffer.from(this.encryptionKey, 'hex'), iv);
        
        let encrypted = cipher.update(JSON.stringify(message), 'utf8', 'hex');
        encrypted += cipher.final('hex');
        
        const authTag = cipher.getAuthTag();
        
        return {
            encrypted: encrypted,
            iv: iv.toString('hex'),
            authTag: authTag.toString('hex')
        };
    }

    // Decrypt message
    decrypt(encryptedData) {
        if (!this.encryptionKey) return encryptedData;
        
        const decipher = crypto.createDecipheriv(
            'aes-256-gcm',
            Buffer.from(this.encryptionKey, 'hex'),
            Buffer.from(encryptedData.iv, 'hex')
        );
        
        decipher.setAuthTag(Buffer.from(encryptedData.authTag, 'hex'));
        
        let decrypted = decipher.update(encryptedData.encrypted, 'hex', 'utf8');
        decrypted += decipher.final('utf8');
        
        return JSON.parse(decrypted);
    }

    // Check access permissions
    checkPermissions(clientId, action, channel) {
        if (!this.accessControlList[clientId]) return false;
        return this.accessControlList[clientId].some(permission => 
            permission.action === action && 
            permission.channel === channel
        );
    }

    // Publish message to a channel
    async publish(clientId, channel, message, options = {}) {
        try {
            // Check permissions
            if (!this.checkPermissions(clientId, 'publish', channel)) {
                throw new Error('Permission denied');
            }

            const messageId = crypto.randomUUID();
            const timestamp = Date.now();

            const messageWrapper = {
                id: messageId,
                timestamp,
                data: message,
                metadata: options.metadata || {},
                retryCount: 0
            };

            // Encrypt if needed
            const finalMessage = this.encrypt(messageWrapper);

            await this.publisher.publish(channel, JSON.stringify(finalMessage));
            this.metrics.messagesPublished++;
            
            this.logger.info('Message published', {
                messageId,
                channel,
                timestamp,
                clientId
            });

            return messageId;
        } catch (error) {
            this.logger.error('Publish error', { error: error.message, channel, clientId });
            throw error;
        }
    }

    // Subscribe to a channel
    async subscribe(clientId, channel, handler, options = {}) {
        if (!this.checkPermissions(clientId, 'subscribe', channel)) {
            throw new Error('Permission denied');
        }

        this.subscriber.subscribe(channel);

        this.subscriber.on('message', async (receivedChannel, message) => {
            if (receivedChannel !== channel) return;

            try {
                const decryptedMessage = this.decrypt(JSON.parse(message));
                this.metrics.messagesConsumed++;

                await handler(decryptedMessage.data, decryptedMessage.metadata);
            } catch (error) {
                this.metrics.messagesFailed++;
                await this.handleFailedMessage(channel, message, error, options);
            }
        });
    }

    // Handle failed message processing
    async handleFailedMessage(channel, message, error, options) {
        const parsedMessage = JSON.parse(message);
        const decryptedMessage = this.decrypt(parsedMessage);

        if (decryptedMessage.retryCount < this.maxRetries) {
            // Retry logic
            const delay = this.retryDelays[decryptedMessage.retryCount];
            decryptedMessage.retryCount++;
            this.metrics.messagesRetried++;

            setTimeout(async () => {
                await this.publisher.publish(channel, 
                    JSON.stringify(this.encrypt(decryptedMessage))
                );
            }, delay);

            this.logger.warn('Message retry scheduled', {
                messageId: decryptedMessage.id,
                retryCount: decryptedMessage.retryCount,
                delay
            });
        } else {
            // Move to Dead Letter Queue
            this.metrics.deadLetters++;
            await this.publisher.lpush(
                `${channel}:dlq`,
                JSON.stringify({
                    message: decryptedMessage,
                    error: error.message,
                    timestamp: Date.now()
                })
            );

            this.logger.error('Message moved to DLQ', {
                messageId: decryptedMessage.id,
                channel,
                error: error.message
            });
        }
    }

    // Get monitoring metrics
    getMetrics() {
        return {
            ...this.metrics,
            timestamp: Date.now()
        };
    }

    // Process messages from Dead Letter Queue
    async processDLQ(channel, handler) {
        const dlqKey = `${channel}:dlq`;
        
        while (true) {
            const message = await this.publisher.rpop(dlqKey);
            if (!message) break;

            try {
                const { message: originalMessage } = JSON.parse(message);
                await handler(originalMessage.data, originalMessage.metadata);
                
                this.logger.info('DLQ message processed successfully', {
                    messageId: originalMessage.id,
                    channel
                });
            } catch (error) {
                // Push back to DLQ if processing fails
                await this.publisher.lpush(dlqKey, message);
                
                this.logger.error('DLQ message processing failed', {
                    error: error.message,
                    channel
                });
                
                break;
            }
        }
    }

    // Graceful shutdown
    async shutdown() {
        await this.publisher.quit();
        await this.subscriber.quit();
        this.logger.info('PubSub system shut down');
    }
}

// Usage example
const config = {
    redis: {
        host: 'localhost',
        port: 6379,
        password: 'your-password'
    },
    encryptionKey: crypto.randomBytes(32).toString('hex'),
    accessControlList: {
        'client-1': [
            { action: 'publish', channel: 'orders' },
            { action: 'subscribe', channel: 'orders' }
        ]
    },
    retryDelays: [1000, 5000, 15000],
    maxRetries: 3
};

const pubSubSystem = new PubSubSystem(config);

// Example publisher
pubSubSystem.publish('client-1', 'orders', {
    orderId: '12345',
    amount: 99.99
}, {
    priority: 'high'
});

// Example subscriber
pubSubSystem.subscribe('client-1', 'orders', async (message, metadata) => {
    console.log('Received order:', message);
    // Process the order
});

// Monitor system metrics
setInterval(() => {
    const metrics = pubSubSystem.getMetrics();
    console.log('System metrics:', metrics);
}, 60000);

// Process Dead Letter Queue
setInterval(async () => {
    await pubSubSystem.processDLQ('orders', async (message, metadata) => {
        // Special handling for failed messages
        console.log('Processing DLQ message:', message);
    });
}, 300000);

module.exports = PubSubSystem;