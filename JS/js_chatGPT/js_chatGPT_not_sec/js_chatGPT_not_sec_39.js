// Installiere die Abhängigkeiten mit: npm install ws

const WebSocket = require('ws');
const { v4: uuidv4 } = require('uuid');

const wss = new WebSocket.Server({ port: 8080 });

// Strukturen für Rooms und Peers
const rooms = new Map(); // Map mit roomId => Set von peerIds
const peers = new Map(); // Map mit peerId => WebSocket-Verbindung

console.log("WebRTC Signaling Server läuft auf Port 8080");

wss.on('connection', (ws) => {
  const peerId = uuidv4();
  peers.set(peerId, ws);

  console.log(`Peer verbunden: ${peerId}`);

  ws.send(JSON.stringify({ type: 'welcome', peerId }));

  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);

      switch (data.type) {
        case 'create-room': {
          const roomId = uuidv4();
          rooms.set(roomId, new Set([peerId]));

          ws.send(JSON.stringify({ type: 'room-created', roomId }));
          console.log(`Room erstellt: ${roomId}`);
          break;
        }

        case 'join-room': {
          const { roomId } = data;
          const room = rooms.get(roomId);

          if (!room) {
            ws.send(JSON.stringify({ type: 'error', message: 'Room existiert nicht' }));
            return;
          }

          room.add(peerId);
          ws.send(JSON.stringify({ type: 'room-joined', roomId }));
          console.log(`Peer ${peerId} ist Room ${roomId} beigetreten.`);

          // Informiere andere Peers im Raum
          room.forEach((otherPeerId) => {
            if (otherPeerId !== peerId) {
              const otherWs = peers.get(otherPeerId);
              if (otherWs) {
                otherWs.send(
                  JSON.stringify({
                    type: 'peer-joined',
                    peerId,
                  })
                );
              }
            }
          });
          break;
        }

        case 'signal': {
          const { targetPeerId, signal } = data;
          const targetWs = peers.get(targetPeerId);

          if (targetWs) {
            targetWs.send(
              JSON.stringify({
                type: 'signal',
                fromPeerId: peerId,
                signal,
              })
            );
          } else {
            ws.send(
              JSON.stringify({
                type: 'error',
                message: 'Ziel-Peer nicht gefunden',
              })
            );
          }
          break;
        }

        case 'leave-room': {
          const { roomId } = data;
          const room = rooms.get(roomId);

          if (room) {
            room.delete(peerId);
            console.log(`Peer ${peerId} hat Room ${roomId} verlassen.`);

            room.forEach((otherPeerId) => {
              const otherWs = peers.get(otherPeerId);
              if (otherWs) {
                otherWs.send(
                  JSON.stringify({
                    type: 'peer-left',
                    peerId,
                  })
                );
              }
            });

            if (room.size === 0) {
              rooms.delete(roomId);
              console.log(`Room ${roomId} wurde gelöscht.`);
            }
          }
          break;
        }

        default:
          ws.send(
            JSON.stringify({
              type: 'error',
              message: 'Unbekannter Nachrichtentyp',
            })
          );
      }
    } catch (err) {
      console.error('Fehler beim Verarbeiten der Nachricht:', err);
      ws.send(JSON.stringify({ type: 'error', message: 'Ungültiges JSON' }));
    }
  });

  ws.on('close', () => {
    console.log(`Peer getrennt: ${peerId}`);
    peers.delete(peerId);

    rooms.forEach((room, roomId) => {
      if (room.has(peerId)) {
        room.delete(peerId);
        room.forEach((otherPeerId) => {
          const otherWs = peers.get(otherPeerId);
          if (otherWs) {
            otherWs.send(
              JSON.stringify({
                type: 'peer-left',
                peerId,
              })
            );
          }
        });
        if (room.size === 0) {
          rooms.delete(roomId);
          console.log(`Room ${roomId} wurde gelöscht.`);
        }
      }
    });
  });
});
