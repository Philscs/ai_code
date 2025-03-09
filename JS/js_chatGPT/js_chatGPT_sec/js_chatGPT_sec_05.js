const WebSocketServerURL = 'wss://example.com/chat';

// Utility Functions
const encryptMessage = (message, key) => {
    // Placeholder for encryption logic (e.g., AES encryption)
    return btoa(JSON.stringify({ message, timestamp: Date.now() }));
};

const sanitizeMessage = (message) => {
    // Sanitize messages to prevent XSS attacks
    const div = document.createElement('div');
    div.innerText = message;
    return div.innerHTML;
};

class SecureWebSocket {
    constructor(url) {
        this.url = url;
        this.ws = null;
        this.heartbeatInterval = 30000; // 30 seconds
        this.reconnectDelay = 5000; // 5 seconds
        this.rateLimitInterval = 1000; // 1 second
        this.maxMessagesPerInterval = 5;
        this.messageCount = 0;
        this.messageQueue = [];
        this.init();
    }

    init() {
        this.ws = new WebSocket(this.url);
        this.ws.onopen = this.onOpen.bind(this);
        this.ws.onmessage = this.onMessage.bind(this);
        this.ws.onclose = this.onClose.bind(this);
        this.ws.onerror = this.onError.bind(this);
        this.startRateLimitTimer();
    }

    onOpen() {
        console.log('WebSocket connection established.');
        this.startHeartbeat();
        this.flushMessageQueue();
    }

    onMessage(event) {
        const data = JSON.parse(event.data);
        console.log('Message received:', sanitizeMessage(data.message));
    }

    onClose() {
        console.log('WebSocket connection closed. Reconnecting...');
        setTimeout(() => this.init(), this.reconnectDelay);
    }

    onError(error) {
        console.error('WebSocket error:', error);
    }

    startHeartbeat() {
        this.heartbeatTimer = setInterval(() => {
            if (this.ws.readyState === WebSocket.OPEN) {
                this.ws.send(JSON.stringify({ type: 'heartbeat' }));
                console.log('Heartbeat sent.');
            }
        }, this.heartbeatInterval);
    }

    stopHeartbeat() {
        clearInterval(this.heartbeatTimer);
    }

    sendMessage(message) {
        if (this.messageCount >= this.maxMessagesPerInterval) {
            console.warn('Rate limit exceeded. Message queued.');
            this.messageQueue.push(message);
            return;
        }

        const encryptedMessage = encryptMessage(message, 'encryption-key');
        if (this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify({ type: 'message', message: encryptedMessage }));
            this.messageCount++;
        } else {
            console.warn('WebSocket not open. Message queued.');
            this.messageQueue.push(message);
        }
    }

    flushMessageQueue() {
        while (this.messageQueue.length > 0 && this.messageCount < this.maxMessagesPerInterval) {
            const message = this.messageQueue.shift();
            this.sendMessage(message);
        }
    }

    startRateLimitTimer() {
        setInterval(() => {
            this.messageCount = 0;
            this.flushMessageQueue();
        }, this.rateLimitInterval);
    }
}

// Example usage
const chatSocket = new SecureWebSocket(WebSocketServerURL);

// Sending a sanitized and encrypted message
const userInput = '<script>alert("XSS")</script>'; // Example malicious input
chatSocket.sendMessage(sanitizeMessage(userInput));
