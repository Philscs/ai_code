// Secure Cross-Origin Communication System

// 1. Define allowed origins
const allowedOrigins = [
    'https://example.com',
    'https://sub.example.com',
  ];
  
  // 2. Function to validate origin
  function isValidOrigin(origin) {
    return allowedOrigins.includes(origin);
  }
  
  // 3. Listener for incoming messages
  window.addEventListener('message', (event) => {
    // Validate origin
    if (!isValidOrigin(event.origin)) {
      console.warn('Rejected message from untrusted origin:', event.origin);
      return;
    }
  
    // Validate message structure (to prevent XSS attacks)
    if (typeof event.data !== 'object' || event.data === null || !event.data.type) {
      console.warn('Invalid message format:', event.data);
      return;
    }
  
    // Process the message securely
    switch (event.data.type) {
      case 'example-action':
        handleExampleAction(event.data.payload);
        break;
      default:
        console.warn('Unknown message type:', event.data.type);
    }
  });
  
  // Example message handler
  function handleExampleAction(payload) {
    console.log('Received payload:', payload);
    // Perform actions securely based on payload
  }
  
  // 4. Function to send messages securely
  function sendMessage(targetWindow, targetOrigin, message) {
    // Validate target origin before sending
    if (!isValidOrigin(targetOrigin)) {
      console.warn('Attempted to send message to untrusted origin:', targetOrigin);
      return;
    }
  
    // Validate message structure before sending
    if (typeof message !== 'object' || message === null || !message.type) {
      throw new Error('Invalid message format');
    }
  
    targetWindow.postMessage(message, targetOrigin);
  }
  
  // 5. Content Security Policy (CSP)
  // Configure in the HTTP headers or <meta> tags of your page:
  // Content-Security-Policy: default-src 'self'; script-src 'self'; connect-src 'self';
  
  // 6. XSS Prevention
  // Ensure that all inputs, data, and messages are sanitized before use
  // Use libraries like DOMPurify for sanitizing HTML if needed
  
  // 7. CSRF Protection
  // Use anti-CSRF tokens for sensitive requests
  // Example: Including a token with messages
  function getCSRFToken() {
    // Generate or retrieve a token securely (e.g., from a cookie or meta tag)
    return document.querySelector('meta[name="csrf-token"]').getAttribute('content');
  }
  
  // Example of sending a message with CSRF token
  function sendSecureMessage(targetWindow, targetOrigin, message) {
    const csrfToken = getCSRFToken();
    const secureMessage = {
      ...message,
      csrfToken,
    };
  
    sendMessage(targetWindow, targetOrigin, secureMessage);
  }
  
  // Usage Example
  // const iframeWindow = document.querySelector('iframe').contentWindow;
  // sendSecureMessage(iframeWindow, 'https://example.com', { type: 'example-action', payload: 'Hello!' });
  