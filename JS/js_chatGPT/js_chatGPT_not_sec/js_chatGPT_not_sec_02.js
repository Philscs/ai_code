// Importiere Abhängigkeiten
const WebSocket = require('ws');

// Initialisiere den WebSocket-Server
const server = new WebSocket.Server({ port: 8080 });

// Speicher für Sitzungen und Aktionen
let sessions = {};

// Funktionen für Undo/Redo
function undoAction(sessionId, userId) {
    const session = sessions[sessionId];
    if (!session || session.history.length === 0) return;

    const lastActionIndex = session.history.findIndex(
        (action) => action.userId === userId
    );
    if (lastActionIndex >= 0) {
        session.redoStack.push(session.history.splice(lastActionIndex, 1)[0]);
    }
}

function redoAction(sessionId, userId) {
    const session = sessions[sessionId];
    if (!session || session.redoStack.length === 0) return;

    const redoActionIndex = session.redoStack.findIndex(
        (action) => action.userId === userId
    );
    if (redoActionIndex >= 0) {
        session.history.push(session.redoStack.splice(redoActionIndex, 1)[0]);
    }
}

// Automatische Konfliktauflösung
function resolveConflicts(history, newAction) {
    // Konfliktlösungslogik (z. B. Priorität nach Zeitstempel oder Benutzer*innen-ID)
    return history;
}

// WebSocket-Serverereignisse
server.on('connection', (ws) => {
    ws.on('message', (message) => {
        const data = JSON.parse(message);

        switch (data.type) {
            case 'join':
                if (!sessions[data.sessionId]) {
                    sessions[data.sessionId] = {
                        history: [],
                        redoStack: [],
                        participants: new Set(),
                    };
                }
                sessions[data.sessionId].participants.add(data.userId);
                ws.send(
                    JSON.stringify({
                        type: 'init',
                        history: sessions[data.sessionId].history,
                    })
                );
                break;

            case 'draw':
                if (!sessions[data.sessionId]) return;
                sessions[data.sessionId].redoStack = []; // Clear redo stack on new action
                sessions[data.sessionId].history.push(data.action);
                sessions[data.sessionId].history = resolveConflicts(
                    sessions[data.sessionId].history,
                    data.action
                );
                broadcast(
                    data.sessionId,
                    JSON.stringify({ type: 'update', action: data.action })
                );
                break;

            case 'undo':
                undoAction(data.sessionId, data.userId);
                broadcast(
                    data.sessionId,
                    JSON.stringify({
                        type: 'sync',
                        history: sessions[data.sessionId].history,
                    })
                );
                break;

            case 'redo':
                redoAction(data.sessionId, data.userId);
                broadcast(
                    data.sessionId,
                    JSON.stringify({
                        type: 'sync',
                        history: sessions[data.sessionId].history,
                    })
                );
                break;

            case 'leave':
                if (sessions[data.sessionId]) {
                    sessions[data.sessionId].participants.delete(data.userId);
                    if (sessions[data.sessionId].participants.size === 0) {
                        delete sessions[data.sessionId];
                    }
                }
                break;
        }
    });

    ws.on('close', () => {
        // Bereinige nicht mehr verbundene Benutzer*innen
        for (const sessionId in sessions) {
            sessions[sessionId].participants.forEach((userId) => {
                // Entferne Benutzer*innen, falls sie keine Verbindung mehr haben
                sessions[sessionId].participants.delete(userId);
            });

            if (sessions[sessionId].participants.size === 0) {
                delete sessions[sessionId];
            }
        }
    });
});

function broadcast(sessionId, message) {
    server.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(message);
        }
    });
}

console.log('WebSocket-Server läuft auf ws://localhost:8080');
