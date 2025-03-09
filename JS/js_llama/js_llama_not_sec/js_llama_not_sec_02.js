// Importiere die notwendigen Bibliotheken
const WebSocket = require('ws');
const { v4: uuidv4 } = require('uuid');

// Definiere eine Klasse für den Whiteboard
class Whiteboard {
  constructor() {
    this.users = [];
    this.drawing = null;
    this.history = [];
    this.currentId = uuidv4();
  }

  // Füge einen neuen Benutzer hinzu
  addUser(user) {
    this.users.push(user);
    user.id = this.currentId;
  }

  // Entferne einen Benutzer
  removeUser(id) {
    const index = this.users.findIndex((user) => user.id === id);
    if (index !== -1) {
      this.users.splice(index, 1);
    }
  }

  // Füge ein Ereignis hinzu
  addEvent(event) {
    this.history.push({
      type: event.type,
      data: event.data,
      timestamp: Date.now(),
    });
  }

  // Verwende die Geschichte für Undo/Redo-Funktionalität
  getHistory() {
    return this.history;
  }
}

// Definiere eine Klasse für das Ereignis
class Event {
  constructor(type, data) {
    this.type = type;
    this.data = data;
  }
}

// Erstelle einen neuen WebSocket-Server
const wss = new WebSocket.Server({ port: 8080 });

// Erstelle ein neues Whiteboard-Objekt
const whiteboard = new Whiteboard();

// Definiere die Ressourcen, die gesendet werden sollen
const resources = {
  draw: (data) => new Event('draw', data),
  undo: () => new Event('undo'),
  redo: () => new Event('redo'),
};

// Konfiguriere den WebSocket-Server
wss.on('connection', (ws) => {
  console.log('Client verbunden');

  // Füge den Benutzer hinzu
  whiteboard.addUser(ws);

  // Registriere Ereignisse
  ws.on('message', (message) => {
    const event = JSON.parse(message);
    switch (event.type) {
      case 'draw':
        whiteboard.addEvent(resources.draw(event.data));
        ws.send(JSON.stringify({ type: 'draw', data: event.data }));
        break;
      case 'undo':
        if (whiteboard.history.length > 0) {
          const lastEvent = whiteboard.history.pop();
          if (lastEvent.type === 'draw') {
            whiteboard.addEvent(resources.undo());
            ws.send(JSON.stringify({ type: 'undo' }));
            wss.clients.forEach((client) => {
              client.send(JSON.stringify(lastEvent));
            });
          }
        } else {
          console.log('Keine weitere Geschichte');
        }
        break;
      case 'redo':
        if (whiteboard.history.length > 0 && whiteboard.history[whiteboard.history.length - 
1].type === 'undo') {
          const lastEvent = whiteboard.history.pop();
          whiteboard.addEvent(resources.redo());
          ws.send(JSON.stringify({ type: 'redo' }));
          wss.clients.forEach((client) => {
            client.send(JSON.stringify(lastEvent));
          });
        } else {
          console.log('Keine weitere Geschichte');
        }
        break;
      default:
        console.log(`Unerwarteter Ereignistyp ${event.type}`);
    }
  });

  // Entferne den Benutzer
  ws.on('close', () => {
    console.log('Client verbunden');
    whiteboard.removeUser(ws.id);
  });
});
