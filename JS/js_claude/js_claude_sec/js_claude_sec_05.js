// Sichere WebSocket-Verbindung mit verschiedenen Sicherheitsfunktionen
class SecureWebSocketClient {
    constructor(url, options = {}) {
      this.url = url;
      this.options = {
        heartbeatInterval: 30000, // 30 Sekunden
        reconnectInterval: 5000,  // 5 Sekunden
        maxReconnectAttempts: 5,
        rateLimit: {
          maxMessages: 10,
          timeWindow: 10000 // 10 Sekunden
        },
        encryptionKey: options.encryptionKey || this.generateEncryptionKey(),
        ...options
      };
  
      this.messageQueue = [];
      this.reconnectAttempts = 0;
      this.lastMessageTimes = [];
      
      this.connect();
    }
  
    // Generiere einen zufälligen Verschlüsselungsschlüssel
    generateEncryptionKey() {
      return Array.from(crypto.getRandomValues(new Uint8Array(32)))
        .map(b => b.toString(16).padStart(2, '0'))
        .join('');
    }
  
    // Verbindung herstellen
    connect() {
      try {
        this.ws = new WebSocket(this.url);
        this.setupEventHandlers();
        this.startHeartbeat();
      } catch (error) {
        console.error('Verbindungsfehler:', error);
        this.handleReconnection();
      }
    }
  
    // Event Handler einrichten
    setupEventHandlers() {
      this.ws.onopen = () => {
        console.log('Verbindung hergestellt');
        this.reconnectAttempts = 0;
        this.processMessageQueue();
      };
  
      this.ws.onclose = () => {
        console.log('Verbindung geschlossen');
        this.handleReconnection();
      };
  
      this.ws.onerror = (error) => {
        console.error('WebSocket Fehler:', error);
      };
  
      this.ws.onmessage = (event) => {
        const decryptedMessage = this.decryptMessage(event.data);
        if (decryptedMessage) {
          this.handleMessage(decryptedMessage);
        }
      };
    }
  
    // Heartbeat-Mechanismus
    startHeartbeat() {
      this.heartbeatInterval = setInterval(() => {
        if (this.ws.readyState === WebSocket.OPEN) {
          this.ws.send(JSON.stringify({ type: 'ping' }));
        }
      }, this.options.heartbeatInterval);
    }
  
    // Nachricht verschlüsseln
    encryptMessage(message) {
      try {
        // In der Praxis würde hier eine echte Verschlüsselung implementiert werden
        // Dies ist nur ein Beispiel
        const encryptedMessage = btoa(JSON.stringify({
          content: message,
          timestamp: Date.now()
        }));
        return encryptedMessage;
      } catch (error) {
        console.error('Verschlüsselungsfehler:', error);
        return null;
      }
    }
  
    // Nachricht entschlüsseln
    decryptMessage(encryptedMessage) {
      try {
        // In der Praxis würde hier eine echte Entschlüsselung implementiert werden
        const decrypted = JSON.parse(atob(encryptedMessage));
        return decrypted.content;
      } catch (error) {
        console.error('Entschlüsselungsfehler:', error);
        return null;
      }
    }
  
    // XSS Prevention
    sanitizeMessage(message) {
      const div = document.createElement('div');
      div.textContent = message;
      return div.innerHTML;
    }
  
    // Rate Limiting
    checkRateLimit() {
      const now = Date.now();
      this.lastMessageTimes = this.lastMessageTimes
        .filter(time => now - time < this.options.rateLimit.timeWindow);
      
      if (this.lastMessageTimes.length >= this.options.rateLimit.maxMessages) {
        return false;
      }
      
      this.lastMessageTimes.push(now);
      return true;
    }
  
    // Nachricht senden
    sendMessage(message) {
      if (!this.checkRateLimit()) {
        console.error('Rate limit überschritten');
        return false;
      }
  
      const sanitizedMessage = this.sanitizeMessage(message);
      const encryptedMessage = this.encryptMessage(sanitizedMessage);
  
      if (!encryptedMessage) {
        return false;
      }
  
      if (this.ws.readyState === WebSocket.OPEN) {
        this.ws.send(encryptedMessage);
        return true;
      } else {
        this.messageQueue.push(encryptedMessage);
        return false;
      }
    }
  
    // Nachrichtenqueue verarbeiten
    processMessageQueue() {
      while (this.messageQueue.length > 0) {
        const message = this.messageQueue.shift();
        if (this.ws.readyState === WebSocket.OPEN) {
          this.ws.send(message);
        } else {
          this.messageQueue.unshift(message);
          break;
        }
      }
    }
  
    // Reconnection Strategy
    handleReconnection() {
      if (this.reconnectAttempts >= this.options.maxReconnectAttempts) {
        console.error('Maximale Anzahl an Reconnection-Versuchen erreicht');
        return;
      }
  
      this.reconnectAttempts++;
      console.log(`Reconnection-Versuch ${this.reconnectAttempts} von ${this.options.maxReconnectAttempts}`);
  
      setTimeout(() => {
        this.connect();
      }, this.options.reconnectInterval);
    }
  
    // Verbindung beenden
    disconnect() {
      if (this.heartbeatInterval) {
        clearInterval(this.heartbeatInterval);
      }
      if (this.ws) {
        this.ws.close();
      }
    }
  
    // Nachrichten verarbeiten
    handleMessage(message) {
      // Hier können eingehende Nachrichten verarbeitet werden
      console.log('Nachricht empfangen:', message);
    }
  }
  
  // Beispiel Verwendung:
  const chatClient = new SecureWebSocketClient('wss://your-websocket-server.com', {
    heartbeatInterval: 20000,
    reconnectInterval: 3000,
    maxReconnectAttempts: 3,
    rateLimit: {
      maxMessages: 5,
      timeWindow: 5000
    }
  });
  
  // Nachricht senden
  chatClient.sendMessage('Hallo Welt!');
  
  // Verbindung beenden
  // chatClient.disconnect();