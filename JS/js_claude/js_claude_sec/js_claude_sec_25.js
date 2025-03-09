// Sichere Cross-Origin Kommunikation

// Whitelist von vertrauenswürdigen Ursprüngen
const TRUSTED_ORIGINS = [
    'https://trusted-domain.com',
    'https://another-trusted.com'
  ];
  
  // Content Security Policy Header
  const CSP_HEADER = {
    'Content-Security-Policy': "default-src 'self'; script-src 'self'; object-src 'none';"
  };
  
  // CSRF Token Generator
  const generateCSRFToken = () => {
    return crypto.randomBytes(32).toString('hex');
  };
  
  // XSS Prevention - Eingabe säubern
  const sanitizeInput = (input) => {
    if (typeof input !== 'string') return input;
    return input
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#x27;')
      .replace(/\//g, '&#x2F;');
  };
  
  // Message-Struktur Validierung
  const validateMessageStructure = (message) => {
    const requiredFields = ['type', 'data', 'timestamp', 'token'];
    return requiredFields.every(field => message.hasOwnProperty(field));
  };
  
  // Origin-Überprüfung
  const isOriginTrusted = (origin) => {
    return TRUSTED_ORIGINS.includes(origin);
  };
  
  // Hauptklasse für sichere Kommunikation
  class SecureCrossOriginMessaging {
    constructor() {
      this.csrfToken = generateCSRFToken();
      this.setupMessageListener();
    }
  
    // Message Listener einrichten
    setupMessageListener() {
      window.addEventListener('message', (event) => {
        this.handleIncomingMessage(event);
      }, false);
    }
  
    // Eingehende Nachrichten verarbeiten
    handleIncomingMessage(event) {
      try {
        // Origin überprüfen
        if (!isOriginTrusted(event.origin)) {
          console.error('Untrusted origin:', event.origin);
          return;
        }
  
        // Nachrichtenstruktur validieren
        if (!validateMessageStructure(event.data)) {
          console.error('Invalid message structure');
          return;
        }
  
        // CSRF Token überprüfen
        if (event.data.token !== this.csrfToken) {
          console.error('Invalid CSRF token');
          return;
        }
  
        // Daten säubern
        const sanitizedData = this.sanitizeMessageData(event.data);
  
        // Nachricht verarbeiten
        this.processMessage(sanitizedData);
  
      } catch (error) {
        console.error('Error processing message:', error);
      }
    }
  
    // Daten in der Nachricht säubern
    sanitizeMessageData(messageData) {
      return {
        ...messageData,
        data: typeof messageData.data === 'object' 
          ? Object.entries(messageData.data).reduce((acc, [key, value]) => {
              acc[sanitizeInput(key)] = sanitizeInput(value);
              return acc;
            }, {})
          : sanitizeInput(messageData.data)
      };
    }
  
    // Nachricht an andere Fenster senden
    sendMessage(targetWindow, message) {
      if (!targetWindow || !message) return;
  
      const secureMessage = {
        ...message,
        timestamp: Date.now(),
        token: this.csrfToken
      };
  
      // Nur an vertrauenswürdige Origins senden
      const targetOrigin = new URL(targetWindow.location.href).origin;
      if (!isOriginTrusted(targetOrigin)) {
        console.error('Cannot send to untrusted origin:', targetOrigin);
        return;
      }
  
      targetWindow.postMessage(secureMessage, targetOrigin);
    }
  
    // Empfangene Nachricht verarbeiten
    processMessage(message) {
      // Hier die eigentliche Nachrichtenverarbeitung implementieren
      console.log('Processing validated message:', message);
    }
  }
  
  // Verwendungsbeispiel:
  const messagingSystem = new SecureCrossOriginMessaging();
  
  // Nachricht senden (Beispiel)
  const message = {
    type: 'USER_ACTION',
    data: { action: 'update', value: 'neue Daten' }
  };
  
  // An ein anderes Fenster senden
  const targetWindow = window.opener || window.parent;
  if (targetWindow) {
    messagingSystem.sendMessage(targetWindow, message);
  }
  
  // Event Listener für eingehende Nachrichten ist bereits aktiv