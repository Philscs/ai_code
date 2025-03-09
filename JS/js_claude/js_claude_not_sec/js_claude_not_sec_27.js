class Message {
    constructor(topic, content, priority = 1) {
      this.id = crypto.randomUUID();
      this.topic = topic;
      this.content = content;
      this.priority = priority;
      this.timestamp = Date.now();
      this.retries = 0;
      this.maxRetries = 3;
    }
  }
  
  class Subscription {
    constructor(topic, callback, filter = null) {
      this.id = crypto.randomUUID();
      this.topic = topic;
      this.callback = callback;
      this.filter = filter;
    }
  }
  
  class MessageBus {
    constructor() {
      // Main message queue for each topic
      this.queues = new Map();
      
      // Dead letter queue for failed messages
      this.deadLetterQueue = new Map();
      
      // Subscriptions for each topic
      this.subscriptions = new Map();
      
      // Channel management
      this.channels = new Set();
      
      // Processing interval in milliseconds
      this.processingInterval = 100;
      
      // Start message processing
      this.startProcessing();
    }
  
    // Channel Management
    createChannel(channelName) {
      if (!this.channels.has(channelName)) {
        this.channels.add(channelName);
        this.queues.set(channelName, []);
        this.subscriptions.set(channelName, []);
      }
      return channelName;
    }
  
    deleteChannel(channelName) {
      this.channels.delete(channelName);
      this.queues.delete(channelName);
      this.subscriptions.delete(channelName);
    }
  
    // Subscription Management
    subscribe(topic, callback, filter = null) {
      if (!this.subscriptions.has(topic)) {
        this.subscriptions.set(topic, []);
      }
      const subscription = new Subscription(topic, callback, filter);
      this.subscriptions.get(topic).push(subscription);
      return subscription.id;
    }
  
    unsubscribe(topic, subscriptionId) {
      if (this.subscriptions.has(topic)) {
        const subs = this.subscriptions.get(topic);
        const index = subs.findIndex(sub => sub.id === subscriptionId);
        if (index !== -1) {
          subs.splice(index, 1);
        }
      }
    }
  
    // Publishing Messages
    publish(topic, content, priority = 1) {
      if (!this.queues.has(topic)) {
        this.queues.set(topic, []);
      }
      
      const message = new Message(topic, content, priority);
      const queue = this.queues.get(topic);
      
      // Insert message based on priority (higher priority first)
      const insertIndex = queue.findIndex(msg => msg.priority < priority);
      if (insertIndex === -1) {
        queue.push(message);
      } else {
        queue.splice(insertIndex, 0, message);
      }
      
      return message.id;
    }
  
    // Message Processing
    startProcessing() {
      setInterval(() => this.processQueues(), this.processingInterval);
    }
  
    async processQueues() {
      for (const [topic, queue] of this.queues.entries()) {
        if (queue.length === 0) continue;
  
        const message = queue[0];
        const subscribers = this.subscriptions.get(topic) || [];
  
        try {
          // Process message for each subscriber
          for (const subscriber of subscribers) {
            if (!subscriber.filter || subscriber.filter(message)) {
              await subscriber.callback(message);
            }
          }
          
          // Remove successfully processed message
          queue.shift();
        } catch (error) {
          message.retries++;
          
          if (message.retries >= message.maxRetries) {
            // Move to dead letter queue
            if (!this.deadLetterQueue.has(topic)) {
              this.deadLetterQueue.set(topic, []);
            }
            this.deadLetterQueue.get(topic).push({
              message,
              error: error.message,
              timestamp: Date.now()
            });
            
            // Remove from main queue
            queue.shift();
          }
        }
      }
    }
  
    // Dead Letter Queue Management
    getDeadLetters(topic) {
      return this.deadLetterQueue.get(topic) || [];
    }
  
    reprocessDeadLetter(topic, messageId) {
      const deadLetters = this.deadLetterQueue.get(topic) || [];
      const index = deadLetters.findIndex(dl => dl.message.id === messageId);
      
      if (index !== -1) {
        const deadLetter = deadLetters[index];
        deadLetter.message.retries = 0;
        this.publish(topic, deadLetter.message.content, deadLetter.message.priority);
        deadLetters.splice(index, 1);
      }
    }
  
    // Utility Methods
    getQueueLength(topic) {
      return (this.queues.get(topic) || []).length;
    }
  
    getSubscriptionCount(topic) {
      return (this.subscriptions.get(topic) || []).length;
    }
  }
  
  // Usage Example
  const messageBus = new MessageBus();
  
  // Create channels
  messageBus.createChannel('notifications');
  messageBus.createChannel('logs');
  
  // Subscribe to channels
  const notificationSub = messageBus.subscribe('notifications', 
    async (message) => {
      console.log(`Processing notification: ${message.content}`);
    },
    (message) => message.priority > 1 // Only process high priority notifications
  );
  
  const logSub = messageBus.subscribe('logs', 
    async (message) => {
      console.log(`Processing log: ${message.content}`);
    }
  );
  
  // Publish messages with different priorities
  messageBus.publish('notifications', 'Urgent notification', 3);
  messageBus.publish('notifications', 'Regular notification', 1);
  messageBus.publish('logs', 'System log entry', 1);