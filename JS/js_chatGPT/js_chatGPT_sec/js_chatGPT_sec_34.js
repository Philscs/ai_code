// Required libraries
const WebSocket = require('ws');
const uuid = require('uuid');
const rateLimit = require('express-rate-limit');

// Server setup
const wss = new WebSocket.Server({ port: 8080 });

// State management
let documents = {}; // Tracks documents and their states
let clients = {};   // Tracks connected clients and their presence

// Helper function: Operational Transform (OT) logic
function applyTransform(document, operation) {
    // Simple OT example for text edits
    const { index, text, type } = operation;
    if (type === 'insert') {
        document = document.slice(0, index) + text + document.slice(index);
    } else if (type === 'delete') {
        document = document.slice(0, index) + document.slice(index + text.length);
    }
    return document;
}

// Rate limiter middleware for API-like control
const limiter = rateLimit({
    windowMs: 1000, // 1 second window
    max: 10,        // limit each client to 10 requests per second
    message: 'Rate limit exceeded, please slow down.',
});

// Security function to validate incoming messages
function isValidOperation(operation) {
    if (!operation || typeof operation !== 'object') return false;
    const { index, text, type } = operation;
    return (
        typeof index === 'number' &&
        typeof text === 'string' &&
        (type === 'insert' || type === 'delete')
    );
}

// Handle client connections
wss.on('connection', (ws) => {
    const clientId = uuid.v4();
    clients[clientId] = { ws, presence: {} };

    // Notify others about the new client
    broadcast({ type: 'presence', clientId, status: 'online' });

    ws.on('message', (message) => {
        try {
            const data = JSON.parse(message);

            if (data.type === 'operation') {
                const { docId, operation } = data;
                if (!isValidOperation(operation)) {
                    ws.send(JSON.stringify({ type: 'error', message: 'Invalid operation.' }));
                    return;
                }

                if (!documents[docId]) {
                    documents[docId] = '';
                }

                // Apply operation with OT
                const updatedDoc = applyTransform(documents[docId], operation);
                documents[docId] = updatedDoc;

                // Broadcast updated document state
                broadcast({
                    type: 'update',
                    docId,
                    document: updatedDoc,
                    operation,
                    clientId,
                });
            } else if (data.type === 'presence') {
                // Update presence information
                clients[clientId].presence = data.presence;
                broadcast({ type: 'presence', clientId, presence: data.presence });
            }
        } catch (err) {
            ws.send(JSON.stringify({ type: 'error', message: 'Invalid message format.' }));
        }
    });

    ws.on('close', () => {
        delete clients[clientId];
        broadcast({ type: 'presence', clientId, status: 'offline' });
    });
});

// Broadcast a message to all connected clients
function broadcast(message) {
    const jsonMessage = JSON.stringify(message);
    Object.values(clients).forEach(({ ws }) => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(jsonMessage);
        }
    });
}

// Middleware integration for rate limiting (if integrated with HTTP API)
function applyRateLimiting(req, res, next) {
    limiter(req, res, next);
}

console.log('Real-Time Collaboration Server running on ws://localhost:8080');
