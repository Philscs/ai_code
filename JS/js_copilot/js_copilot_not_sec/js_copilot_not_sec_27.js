// BEGIN: Message Bus

class MessageBus {
  constructor() {
    this.channels = new Map();
    this.deadLetterQueue = [];
  }

  subscribe(channel, callback) {
    if (!this.channels.has(channel)) {
      this.channels.set(channel, []);
    }
    this.channels.get(channel).push(callback);
  }

  unsubscribe(channel, callback) {
    if (this.channels.has(channel)) {
      const subscribers = this.channels.get(channel);
      const index = subscribers.indexOf(callback);
      if (index !== -1) {
        subscribers.splice(index, 1);
      }
    }
  }

  publish(channel, message, priority = 0) {
    if (this.channels.has(channel)) {
      const subscribers = this.channels.get(channel);
      subscribers.sort((a, b) => b.priority - a.priority);
      subscribers.forEach(callback => callback(message));
    } else {
      this.deadLetterQueue.push({ channel, message });
    }
  }

  processDeadLetterQueue() {
    this.deadLetterQueue.forEach(({ channel, message }) => {
      if (this.channels.has(channel)) {
        const subscribers = this.channels.get(channel);
        subscribers.sort((a, b) => b.priority - a.priority);
        subscribers.forEach(callback => callback(message));
      }
    });
    this.deadLetterQueue = [];
  }
}

// END: Message Bus
