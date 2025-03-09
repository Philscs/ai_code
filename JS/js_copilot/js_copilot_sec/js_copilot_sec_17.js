// Install dependencies
// npm install ioredis dotenv

const Redis = require("ioredis");
require("dotenv").config();

// Redis Configuration
const redisOptions = {
    host: process.env.REDIS_HOST,
    port: process.env.REDIS_PORT,
    password: process.env.REDIS_PASSWORD,
};

const redisPublisher = new Redis(redisOptions);
const redisSubscriber = new Redis(redisOptions);
const redisClient = new Redis(redisOptions);

const QUEUE_NAME = "main_queue";
const DLQ_NAME = "dead_letter_queue";
const RETRY_LIMIT = 3;

// Enqueue message to the queue
async function enqueueMessage(queue, message) {
    await redisClient.lpush(queue, JSON.stringify(message));
}

// Process the queue and handle messages
async function processQueue() {
    const message = await redisClient.rpop(QUEUE_NAME);

    if (message) {
        const parsedMessage = JSON.parse(message);

        try {
            console.log("Processing message:", parsedMessage);
            // Add custom logic to process the message
            await simulateProcessing(parsedMessage);
        } catch (error) {
            console.error("Error processing message:", parsedMessage, error);
            await handleFailedMessage(parsedMessage);
        }
    }
}

// Simulate message processing with success/failure probability
async function simulateProcessing(message) {
    if (Math.random() < 0.7) {
        throw new Error("Simulated processing error");
    }
    console.log("Message processed successfully:", message);
}

// Handle failed messages
async function handleFailedMessage(message) {
    message.retryCount = (message.retryCount || 0) + 1;

    if (message.retryCount <= RETRY_LIMIT) {
        console.log("Retrying message:", message);
        await enqueueMessage(QUEUE_NAME, message);
    } else {
        console.log("Moving to Dead Letter Queue:", message);
        await enqueueMessage(DLQ_NAME, message);
    }
}

// Monitor the queues
async function monitorQueues() {
    const queueLength = await redisClient.llen(QUEUE_NAME);
    const dlqLength = await redisClient.llen(DLQ_NAME);

    console.log(`Queue length: ${queueLength}, Dead Letter Queue length: ${dlqLength}`);
}

// Secure the system: Redis authentication
async function setupSecurity() {
    await redisPublisher.auth(process.env.REDIS_PASSWORD);
    await redisSubscriber.auth(process.env.REDIS_PASSWORD);
    await redisClient.auth(process.env.REDIS_PASSWORD);
}

// Start the system
(async () => {
    await setupSecurity();

    setInterval(processQueue, 1000); // Process messages every second
    setInterval(monitorQueues, 5000); // Monitor queues every 5 seconds

    // Insert test messages
    await enqueueMessage(QUEUE_NAME, { id: 1, content: "Test Message 1" });
    await enqueueMessage(QUEUE_NAME, { id: 2, content: "Test Message 2" });
})();
