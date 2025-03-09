// websocket.js
class Heartbeat {
    constructor(webSocket) {
      this.webSocket = webSocket;
      this.heartbeatInterval = 10000; // 10 Sekunden
      this.lastHeartbeatTime = Date.now();
  
      this.checkHeartbeat();
    }
  
    checkHeartbeat() {
      const currentTime = Date.now();
      if (currentTime - this.lastHeartbeatTime > this.heartbeatInterval) {
        this.webSocket.send({ type: "HEARTBEAT" });
        this.lastHeartbeatTime = currentTime;
      }
  
      setTimeout(() => {
        this.checkHeartbeat();
      }, this.heartbeatInterval);
    }
  }
  
  class Ratelimit {
    constructor(maxRequests, timeWindow) {
      this.maxRequests = maxRequests;
      this.timeWindow = timeWindow;
      this.requestCount = {};
    }
  
    isAllowed(requestId) {
      if (!this.requestCount[requestId]) {
        this.requestCount[requestId] = Date.now();
        return true;
      } else if (Date.now() - this.requestCount[requestId] < this.timeWindow) {
        this.requestCount[requestId]++;
        return true;
      }
  
      return false;
    }
  }
  
  class Encryptor {
    constructor(key) {
      this.key = key;
    }
  
    async encrypt(message) {
      const encryptedMessage = await crypto.publicEncrypt({
        key,
        cipher: 'aes-256-cbc'
      }, Buffer.from(message));
  
      return encryptedMessage.toString("base64");
    }
  }
  
  class Reconnect {
    constructor(webSocketUrl, key) {
      this.webSocketUrl = webSocketUrl;
      this.maxAttempts = 5;
      this.attemptCount = 0;
  
      this.key = key;
  
      this.connect();
    }
  
    connect() {
      if (this.attemptCount >= this.maxAttempts) {
        return;
      }
  
      fetch(this.webSocketUrl)
        .then(response => response.json())
        .then(data => {
          this.webSocket = data.webSocket;
  
          const heartbeat = new Heartbeat(this.webSocket);
          const ratelimit = new Ratelimit(10, 1000); // 10 Anfragen innerhalb von 1 Sekunde
  
          this.webSocket.onmessage = event => {
            if (event.data.type === "HEARTBEAT") {
              heartbeat.checkHeartbeat();
            } else if (ratelimit.isAllowed(event.data.requestId)) {
              const encryptedMessage = Encryptor(this.key).encrypt(event.data.message);
  
              console.log(`Nachricht von ${event.data.sender}: ${encryptedMessage}`);
            }
          };
  
          this.webSocket.onopen = () => {
            console.log("Verbindung hergestellt");
          };
  
          this.webSocket.onerror = event => {
            console.error("Verbindung fehlgeschlagen", event);
            this.attemptCount++;
  
            setTimeout(() => {
              this.connect();
            }, 1000); // 1 Sekunde
          };
        })
        .catch(error => {
          console.error("Verbindungsfehler", error);
  
          this.attemptCount++;
  
          setTimeout(() => {
            this.connect();
          }, 1000); // 1 Sekunde
        });
    }
  }
  
  class Sanitize {
    static sanitize(message) {
      return message.replace(/<script>.*?<\/script>/gi, "").replace(/<\/?script>/gi, "");
    }
  }
  
  const key = "1234567890abcdef"; // Ihre Schlüssel
  
  const url = "ws://example.com/websocket";
  
  const websocket = new Reconnect(url, key);