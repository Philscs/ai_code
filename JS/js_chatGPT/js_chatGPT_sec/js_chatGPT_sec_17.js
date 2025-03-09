// Installieren der Abhängigkeiten
// npm install ioredis dotenv

const Redis = require("ioredis");
require("dotenv").config();

// Redis Konfiguration
const redisPublisher = new Redis(process.env.REDIS_URL);
const redisSubscriber = new Redis(process.env.REDIS_URL);
const redisClient = new Redis(process.env.REDIS_URL);

const QUEUE_NAME = "main_queue";
const DLQ_NAME = "dead_letter_queue";
const RETRY_LIMIT = 3;

// Nachricht in die Queue einfügen
async function enqueueMessage(queue, message) {
  await redisClient.lpush(queue, JSON.stringify(message));
}

// Nachricht aus der Queue lesen und verarbeiten
async function processQueue() {
  const message = await redisClient.rpop(QUEUE_NAME);

  if (message) {
    const parsedMessage = JSON.parse(message);

    try {
      console.log("Processing message:", parsedMessage);
      // Nachricht verarbeiten (benutzerdefinierte Logik einbauen)
      await simulateProcessing(parsedMessage);
    } catch (error) {
      console.error("Error processing message:", parsedMessage, error);
      await handleFailedMessage(parsedMessage);
    }
  }
}

// Nachricht verarbeiten (Simulation mit Erfolgs-/Fehlerwahrscheinlichkeit)
async function simulateProcessing(message) {
  if (Math.random() < 0.7) {
    throw new Error("Simulated processing error");
  }
  console.log("Message processed successfully:", message);
}

// Fehlgeschlagene Nachricht behandeln
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

// Monitoring der Queues
async function monitorQueues() {
  const queueLength = await redisClient.llen(QUEUE_NAME);
  const dlqLength = await redisClient.llen(DLQ_NAME);

  console.log(`Queue length: ${queueLength}, Dead Letter Queue length: ${dlqLength}`);
}

// Sicherheitskontrollen: Redis-Authentifizierung
async function setupSecurity() {
  redisPublisher.auth(process.env.REDIS_PASSWORD);
  redisSubscriber.auth(process.env.REDIS_PASSWORD);
  redisClient.auth(process.env.REDIS_PASSWORD);
}

// Start des Systems
(async () => {
  await setupSecurity();

  setInterval(processQueue, 1000); // Verarbeite Nachrichten jede Sekunde
  setInterval(monitorQueues, 5000); // Monitoring alle 5 Sekunden

  // Testnachrichten einfügen
  await enqueueMessage(QUEUE_NAME, { id: 1, content: "Test Message 1" });
  await enqueueMessage(QUEUE_NAME, { id: 2, content: "Test Message 2" });
})();
