const WebSocket = require('ws');
const { v4: uuidv4 } = require('uuid');

class WhiteboardServer {
    constructor(port = 8080) {
        this.wss = new WebSocket.Server({ port });
        this.clients = new Map(); // client -> {id, history}
        this.history = []; // Globaler Verlauf aller Zeichenoperationen
        this.undoHistory = new Map(); // client -> [[operations]]
        
        this.setupWebSocket();
        console.log(`Whiteboard server running on port ${port}`);
    }

    setupWebSocket() {
        this.wss.on('connection', (ws) => {
            const clientId = uuidv4();
            this.clients.set(ws, {
                id: clientId,
                history: []
            });

            // Sende aktuelle History an neuen Client
            ws.send(JSON.stringify({
                type: 'init',
                history: this.history,
                clientId
            }));

            ws.on('message', (message) => {
                try {
                    const data = JSON.parse(message);
                    this.handleMessage(ws, data);
                } catch (error) {
                    console.error('Error handling message:', error);
                }
            });

            ws.on('close', () => {
                this.clients.delete(ws);
                this.undoHistory.delete(clientId);
                this.broadcastUserList();
            });

            this.broadcastUserList();
        });
    }

    handleMessage(ws, data) {
        const client = this.clients.get(ws);
        
        switch (data.type) {
            case 'draw':
                this.handleDrawOperation(client, data);
                break;
            case 'undo':
                this.handleUndo(client);
                break;
            case 'redo':
                this.handleRedo(client);
                break;
            default:
                console.warn('Unknown message type:', data.type);
        }
    }

    handleDrawOperation(client, data) {
        const operation = {
            ...data,
            clientId: client.id,
            timestamp: Date.now(),
            operationId: uuidv4()
        };

        // Füge Operation zur globalen History hinzu
        this.history.push(operation);
        client.history.push(operation);

        // Lösche mögliche Redo-Operationen
        const clientUndoHistory = this.undoHistory.get(client.id) || [];
        if (clientUndoHistory.length > 0) {
            clientUndoHistory.length = 0;
        }

        // Broadcast an alle Clients außer Sender
        this.broadcast(operation, client.id);
    }

    handleUndo(client) {
        if (client.history.length === 0) return;

        const lastOperation = client.history.pop();
        const clientUndoHistory = this.undoHistory.get(client.id) || [];
        clientUndoHistory.push(lastOperation);
        this.undoHistory.set(client.id, clientUndoHistory);

        // Entferne Operation aus globaler History
        const index = this.history.findIndex(op => op.operationId === lastOperation.operationId);
        if (index !== -1) {
            this.history.splice(index, 1);
        }

        // Broadcast Undo
        this.broadcast({
            type: 'undo',
            clientId: client.id,
            operationId: lastOperation.operationId
        });
    }

    handleRedo(client) {
        const clientUndoHistory = this.undoHistory.get(client.id) || [];
        if (clientUndoHistory.length === 0) return;

        const operation = clientUndoHistory.pop();
        client.history.push(operation);
        this.history.push(operation);

        // Broadcast Redo
        this.broadcast({
            type: 'redo',
            clientId: client.id,
            operation
        });
    }

    broadcast(data, excludeClientId = null) {
        this.clients.forEach((client, ws) => {
            if (client.id !== excludeClientId) {
                ws.send(JSON.stringify(data));
            }
        });
    }

    broadcastUserList() {
        const users = Array.from(this.clients.values()).map(client => ({
            id: client.id
        }));

        this.broadcast({
            type: 'users',
            users
        });
    }

    // Konfliktauflösung basierend auf Timestamps
    resolveConflict(op1, op2) {
        return op1.timestamp <= op2.timestamp;
    }
}

// Server starten
const server = new WhiteboardServer();

// Example Client Usage:
/*
const ws = new WebSocket('ws://localhost:8080');

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    switch(data.type) {
        case 'init':
            // Initialize canvas with existing history
            break;
        case 'draw':
            // Add new drawing operation
            break;
        case 'undo':
            // Remove specified operation
            break;
        case 'redo':
            // Reapply operation
            break;
        case 'users':
            // Update user list
            break;
    }
};

// Send drawing operation
ws.send(JSON.stringify({
    type: 'draw',
    x: 100,
    y: 100,
    color: '#000000',
    thickness: 2
}));
*/