class PeerDiscovery {
    constructor() {
      this.peers = {};
    }
  
    addPeer(peerId) {
      this.peers[peerId] = new WebSocket(`ws://${peerId}`);
    }
  
    removePeer(peerId) {
      if (this.peers[peerId]) {
        this.peers[peerId].close();
        delete this.peers[peerId];
      }
    }
  
    isConnected(peerId) {
      return this.peers[peerId] !== undefined;
    }
  
    receiveMessage(peerId, message) {
      if (this.isConnected(peerId)) {
        this.peers[peerId].send(message);
      }
    }
  }
  
  class DataChannel {
    constructor() {
      this.channel = null;
    }
  
    createChannel(peerId) {
      const socket = new WebSocket(`ws://${peerId}`);
      this.channel = new RTCPeerConnection(socket);
    }
  
    sendMessage(data) {
      if (this.channel) {
        this.channel.send(data);
      }
    }
  
    receiveMessage() {
      return new Promise((resolve, reject) => {
        this.channel.onmessage = (event) => {
          resolve(event.data);
        };
        this.channel.onerror = (event) => {
          reject(new Error('Error occurred while receiving data'));
        };
        this.channel.onclose = () => {
          this.close();
        };
      });
    }
  
    close() {
      if (this.channel) {
        this.channel.close();
        this.channel = null;
      }
    }
  }
  
  class Security {
    constructor() {}
    createConnection(socketUrl) {
      const socket = new WebSocket(socketUrl);
      console.log('Security: Connection established');
    }
  }
  
  class ResourceManager {
    constructor() {}
    getResourceStatus() {
      return {};
    }
  }
  
  class RateLimiter {
    constructor() {
      this.currentDataTransferRate = {};
    }
  
    addDataTransfer(peerId, data) {
      const currentTime = new Date().getTime();
      if (this.currentDataTransferRate[peerId]) {
        const previousTimestamp = this.currentDataTransferRate[peerId];
        const timeElapsed = (currentTime - previousTimestamp) / 1000;
        const rate = data.length / timeElapsed;
  
        // Rate-Limitierung
        if (rate > 10 * 1024 * 1024) { // 10 MB/s
          throw new Error('Überhöhte Datenübertragungsrate');
        }
  
        this.currentDataTransferRate[peerId] = currentTime;
      } else {
        this.currentDataTransferRate[peerId] = currentTime;
      }
    }
  
    getResourceStatus() {
      return this.currentDataTransferRate;
    }
  }
  
  class Application {
    constructor() {
      this.peerDiscovery = new PeerDiscovery();
      this.dataChannel = new DataChannel();
      this.security = new Security();
      this.resourceManager = new ResourceManager();
      this.rateLimiter = new RateLimiter();
  
      this.createPeer('peer1');
      this.createPeer('peer2');
  
      console.log('Sending data...');
      const data = 'Hello, world!';
      this.rateLimiter.addDataTransfer('peer1', data);
      this.dataChannel.sendMessage(data);
  
      console.log('Receiving data...');
      return this.dataChannel.receiveMessage();
    }
  
    createPeer(peerId) {
      this.peerDiscovery.addPeer(peerId);
      const socketUrl = `ws://${peerId}`;
      this.security.createConnection(socketUrl);
    }
  }
  
  const app = new Application();
  
  app.receiveMessage().then((message) => {
    console.log(message); // "Hello, world!"
  }).catch((error) => {
    console.error(error);
  });
  