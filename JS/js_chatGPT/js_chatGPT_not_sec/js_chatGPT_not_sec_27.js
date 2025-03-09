class MessageBus {
    constructor() {
      this.channels = new Map(); // Verwaltet Kanäle und ihre Abonnent*innen
      this.queues = new Map(); // Nachrichtenschlangen pro Kanal
      this.deadLetterQueues = new Map(); // Dead Letter Queues pro Kanal
    }
  
    // Neuen Kanal erstellen
    createChannel(channel) {
      if (!this.channels.has(channel)) {
        this.channels.set(channel, new Set());
        this.queues.set(channel, []);
        this.deadLetterQueues.set(channel, []);
      }
    }
  
    // Abonnieren eines Kanals
    subscribe(channel, subscriber) {
      if (!this.channels.has(channel)) {
        this.createChannel(channel);
      }
      this.channels.get(channel).add(subscriber);
    }
  
    // Abbestellen eines Kanals
    unsubscribe(channel, subscriber) {
      if (this.channels.has(channel)) {
        this.channels.get(channel).delete(subscriber);
      }
    }
  
    // Nachricht mit Priorität veröffentlichen
    publish(channel, message, priority = 0) {
      if (!this.channels.has(channel)) {
        this.createChannel(channel);
      }
      const queue = this.queues.get(channel);
      queue.push({ message, priority });
      queue.sort((a, b) => b.priority - a.priority); // Nachrichten mit höherer Priorität kommen zuerst
    }
  
    // Nachrichten verarbeiten
    processMessages(channel) {
      if (!this.channels.has(channel)) {
        console.warn(`Channel ${channel} existiert nicht.`);
        return;
      }
  
      const queue = this.queues.get(channel);
      const subscribers = this.channels.get(channel);
  
      while (queue.length > 0) {
        const { message } = queue.shift();
        let delivered = false;
  
        for (const subscriber of subscribers) {
          try {
            subscriber(message);
            delivered = true;
          } catch (error) {
            console.error(`Fehler beim Senden der Nachricht: ${error.message}`);
          }
        }
  
        if (!delivered) {
          this.deadLetterQueues.get(channel).push(message);
        }
      }
    }
  
    // Dead Letter Queue abfragen
    getDeadLetterQueue(channel) {
      if (this.deadLetterQueues.has(channel)) {
        return this.deadLetterQueues.get(channel);
      }
      return [];
    }
  }
  
  // Beispielnutzung
  const bus = new MessageBus();
  
  // Kanal erstellen
  bus.createChannel("news");
  
  // Abonnieren
  bus.subscribe("news", (msg) => {
    console.log(`Empfangen: ${msg}`);
  });
  
  // Nachricht veröffentlichen
  bus.publish("news", "Wichtige Nachricht", 1);
  bus.publish("news", "Normale Nachricht", 0);
  
  // Nachrichten verarbeiten
  bus.processMessages("news");
  
  // Dead Letter Queue prüfen
  console.log("Dead Letter Queue:", bus.getDeadLetterQueue("news"));
  