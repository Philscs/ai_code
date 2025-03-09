// json-stream.js

class JSONStream {
    constructor(data, transform, filter) {
      this.data = data;
      this.transform = transform || (data => data);
      this.filter = filter || (data => true);
  
      // Initialize the observer array
      this.observers = [];
    }
  
    addSubscriber(subscriber) {
      this.observers.push(subscriber);
    }
  
    nextChunk(chunk) {
      // Transform and filter the chunk
      const transformedChunk = this.transform(chunk);
      if (!this.filter(transformedChunk)) return;
  
      // Notify all subscribers
      this.observers.forEach(observer => observer.next(transformedChunk));
    }
  }
  
  class JSONTransformer extends JSONStream {
    transform(chunk) {
      return chunk.map(item => ({
        ...item,
        age: item.age / 2
      }));
    }
  }
  
  class JSONFilterer extends JSONStream {
    filter(chunk) {
      return chunk.every(item => item.name.startsWith('J'));
    }
  }
  
  // HTTP-Server
  const http = require('http');
  
  function startServer() {
    // Listen auf Port 3000
    const server = http.createServer((request, response) => {
      // Erstellt einen Stream für die Ausgabe
      const jsonStream = new JSONStream('');
  
      // Erstellt eine Verbindung zu dem Client (z.B. über WebSockets)
      const clientConnection = request.socket;
  
      // Sendet das erste Chunk des JSON-Streams
      jsonStream.nextChunk(jsonContent);
  
      // Hört auf die Anforderungen des Clients und sendet den nächsten Chunk
      clientConnection.on('data', chunk => {
        if (!chunk) return;
        jsonStream.nextChunk(chunk.toString());
      });
  
      response.writeHead(200, {'Content-Type': 'application/json'});
    });
  
    server.listen(3000, () => {
      console.log('Server gestartet');
    });
  }
  
  // JSON-Datei als String
  const jsonContent = `
  [
    {"name": "John", "age": 30},
    {"name": "Jane", "age": 25},
    {"name": "Bob", "age": 40}
  ]
  `;
  
  startServer();
  
  // Beispiele für Abonnenten
  class JSONSubscriber {
    next(data) {
      console.log('Received data:', data);
    }
  }
  
  const subscriber1 = new JSONSubscriber();
  const subscriber2 = new JSONSubscriber();
  
  jsonStream.addSubscriber(subscriber1);
  jsonStream.addSubscriber(subscriber2);
  
  // Beispiel für die Nutzung von Transformatoren und Filterern
  startServer();
  