class Message {
    constructor(id, content) {
      this.id = id;
      this.content = content;
    }
  }
  
  class Channel {
    constructor(name) {
      this.name = name;
      this.messages = [];
    }
  
    addMessage(message) {
      this.messages.push(message);
    }
  
    getMessages() {
      return this.messages;
    }
  
    getChannelName() {
      return this.name;
    }
  }
  
  class MessageBus {
    constructor() {
      this.channels = {};
      this.deadLetterQueue = {};
      this.messagePrioritization = {};
    }
  
    addChannel(channelName) {
      if (!this.channels[channelName]) {
        this.channels[channelName] = new Channel(channelName);
      }
    }
  
    publish(message, channelName) {
      const messageBus = this.channels[channelName];
      if (messageBus) {
        messageBus.addMessage(message);
      } else {
        console.error(`Channel ${channelName} does not exist`);
      }
    }
  
    subscribe(channelName, callback) {
      const channel = this.channels[channelName];
      if (channel) {
        channel.addMessage((message) => {
          callback(message.content);
        });
      } else {
        console.error(`Channel ${channelName} does not exist`);
      }
    }
  
    getMessagesFromQueue(queueName) {
      return this.deadLetterQueue[queueName].messages;
    }
  
    putMessageInDeadLetterQueue(message, queueName) {
      if (!this.deadLetterQueue[queueName]) {
        this.deadLetterQueue[queueName] = { messages: [] };
      }
      const messageBus = this.channels[message.channelName];
      if (messageBus) {
        const index = messageBus.messages.findIndex((m) => m.id === message.id);
        if (index !== -1) {
          delete messageBus.messages[index];
        } else {
          console.error(`Message with id ${message.id} does not exist`);
        }
        this.deadLetterQueue[queueName].messages.push(message);
      } else {
        console.error(`Channel ${message.channelName} does not exist`);
      }
    }
  
    putPrioritizedMessageInChannel(channelName, message) {
      const channel = this.channels[channelName];
      if (channel) {
        const index = channel.messages.findIndex((m) => m.content === message.content);
        if (index !== -1) {
          delete channel.messages[index];
        } else {
          console.error(`Message with content ${message.content} does not exist`);
        }
        this.messagePrioritization[channelName] = Math.random();
        channel.addMessage(message);
      } else {
        console.error(`Channel ${channelName} does not exist`);
      }
    }
  
    getHighestPriorityChannel() {
      const highestPriorityChannel = Object.keys(this.messagePrioritization).reduce((a, b) => {
        return this.messagePrioritization[a] > this.messagePrioritization[b] ? a : b;
      });
      return this.channels[highestPriorityChannel];
    }
  }
  
  export default MessageBus;
  const messageBus = new MessageBus();
  
  messageBus.addChannel('orders');
  
  // Produktion von Nachrichten
  const orderMessage1 = new Message(1, 'Order 1');
  messageBus.publish(orderMessage1, 'orders');
  
  const orderMessage2 = new Message(2, 'Order 2');
  messageBus.putPrioritizedMessageInChannel('orders', orderMessage2);
  
  // Abonnieren von Nachrichten
  messageBus.subscribe('orders', (content) => {
    console.log(`Received Order: ${content}`);
  });
  
  // Rücksendung von Nachrichten in die Dead-Letter-Queue
  const deadLetterQueueMessage1 = new Message(3, 'Dead-Letter-Message 1');
  messageBus.putMessageInDeadLetterQueue(deadLetterQueueMessage1, 'dead-letter-queue');
  
  // Holen von Nachrichten aus der Dead-Letter-Queue
  console.log(messageBus.getMessagesFromQueue('dead-letter-queue').map((m) => m.content));
  