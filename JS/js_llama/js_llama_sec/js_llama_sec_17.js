const redis = require('redis');
const client = redis.createClient({
  host: 'localhost',
  port: 6379,
  db: 0
});

class PubSubSystem {
  constructor() {
    this.client = client;
    this.deadLetterQueue = '';
    this.monitoringInterval = null;

    // Sicherheitskontrolle
    this.securityToken = '';

    // Setzen des Sicherheitstokens für die Verbindung
    client.auth(this.securityToken, (err) => {
      if (err) {
        console.error(err);
      } else {
        console.log('Verbindung mit Redis hergestellt');
      }
    });
  }

  // Publikation eines Nachrichtensatzes
  publish(messageType, payload) {
    const messageKey = `message:${messageType}`;
    this.client.publish(messageKey, JSON.stringify(payload));

    if (this.monitoringInterval) {
      clearInterval(this.monitoringInterval);
    }

    this.monitoringInterval = setInterval(() => {
      this.monitoring();
    }, 1000); // 1 Sekunde
  }

  // Abholen einer Nachricht aus der Publikations-Queue
  listen(messageType, callback) {
    const messageKey = `message:${messageType}`;
    this.client.subscribe(messageKey);

    this.client.on('message', (channel, message) => {
      const payload = JSON.parse(message);
      if (!callback || !callback(payload)) return;

      callback();
    });
  }

  // Abholen einer Nachricht aus der Dead-Letter-Queue
  listenDeadLetter(callback) {
    const deadLetterKey = 'dead-letter';
    this.client.subscribe(deadLetterKey);

    this.client.on('message', (channel, message) => {
      const payload = JSON.parse(message);
      if (!callback || !callback(payload)) return;

      callback();
    });
  }

  // Überprüfen, ob eine Nachricht in der Dead-Letter-Queue abgeleitet wurde
  checkDeadLetter() {
    this.client.hget('dead-letter', 'count');
  }

  // Abholen eines abgeleiteten Nachrichtensatzes aus der Dead-Letter-Queue
  recover(messageType) {
    const messageKey = `message:${messageType}`;
    this.client.get(messageKey, (err, reply) => {
      if (err || !reply) return;

      console.log('Abgeleitet: ', JSON.parse(reply));
    });
  }

  // Überprüfen der Gesamtzahl der Nachrichten in der Publikations-Queue
  monitoring() {
    const messageCount = this.client.keys('*', (err, keys) => {
      if (err) return;

      console.log('Gesamtzahl Nachrichten: ', keys.length);
    });
  }
}

// Erstellen des Pub/Sub-Systems
const pubSubSystem = new PubSubSystem();

// Sicherheitskontrolle
pubSubSystem.securityToken = '1234567890abcdef';

// Abholen einer Nachricht aus der Publikations-Queue
pubSubSystem.listen('message:hello', (payload) => {
  console.log(payload);
});

// Publikation eines Nachrichtensatzes
pubSubSystem.publish('message:hello', { message: 'Hallo Welt!' });

// Abholen eines abgeleiteten Nachrichtensatzes aus der Dead-Letter-Queue
pubSubSystem.listenDeadLetter((payload) => {
  console.log(payload);
});

// Überprüfen, ob eine Nachricht in der Dead-Letter-Queue abgeleitet wurde
pubSubSystem.checkDeadLetter();

// Abholen eines abgeleiteten Nachrichtensatzes aus der Dead-Letter-Queue
pubSubSystem.recover('message:hello');