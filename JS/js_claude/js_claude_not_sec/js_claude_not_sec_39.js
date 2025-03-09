const express = require('express');
const WebSocket = require('ws');
const { v4: uuidv4 } = require('uuid');

const app = express();
const server = require('http').createServer(app);
const wss = new WebSocket.Server({ server });

// Speichert alle aktiven Verbindungen
const connections = new Map();
// Speichert alle Räume und ihre Teilnehmer
const rooms = new Map();

// Hilfsfunktion zum Senden von Nachrichten
function sendToClient(client, type, payload) {
  if (client.readyState === WebSocket.OPEN) {
    client.send(JSON.stringify({ type, payload }));
  }
}

// Broadcast an alle Clients in einem Raum
function broadcastToRoom(roomId, senderId, type, payload) {
  const room = rooms.get(roomId);
  if (room) {
    room.forEach(clientId => {
      if (clientId !== senderId) {
        const client = connections.get(clientId);
        if (client) {
          sendToClient(client, type, payload);
        }
      }
    });
  }
}

wss.on('connection', (ws) => {
  // Generiere eindeutige ID für den Client
  const clientId = uuidv4();
  connections.set(clientId, ws);

  // Sende Client ID zurück
  sendToClient(ws, 'connected', { clientId });

  ws.on('message', (message) => {
    let data;
    try {
      data = JSON.parse(message);
    } catch (e) {
      console.error('Ungültiges Nachrichtenformat:', e);
      return;
    }

    switch (data.type) {
      case 'join_room':
        // Client tritt einem Raum bei
        const { roomId } = data.payload;
        if (!rooms.has(roomId)) {
          rooms.set(roomId, new Set());
        }
        rooms.get(roomId).add(clientId);

        // Informiere andere Teilnehmer über den neuen Peer
        broadcastToRoom(roomId, clientId, 'peer_joined', { peerId: clientId });

        // Sende Liste aller Peers im Raum an den neuen Client
        const peers = Array.from(rooms.get(roomId)).filter(id => id !== clientId);
        sendToClient(ws, 'room_peers', { peers });
        break;

      case 'leave_room':
        // Client verlässt einen Raum
        const roomToLeave = data.payload.roomId;
        if (rooms.has(roomToLeave)) {
          rooms.get(roomToLeave).delete(clientId);
          if (rooms.get(roomToLeave).size === 0) {
            rooms.delete(roomToLeave);
          }
          broadcastToRoom(roomToLeave, clientId, 'peer_left', { peerId: clientId });
        }
        break;

      case 'offer':
        // Weiterleiten des SDP Offers
        const { target: offerTarget, sdp: offerSdp } = data.payload;
        const offerPeer = connections.get(offerTarget);
        if (offerPeer) {
          sendToClient(offerPeer, 'offer', {
            sdp: offerSdp,
            peerId: clientId
          });
        }
        break;

      case 'answer':
        // Weiterleiten der SDP Answer
        const { target: answerTarget, sdp: answerSdp } = data.payload;
        const answerPeer = connections.get(answerTarget);
        if (answerPeer) {
          sendToClient(answerPeer, 'answer', {
            sdp: answerSdp,
            peerId: clientId
          });
        }
        break;

      case 'ice_candidate':
        // Weiterleiten der ICE Kandidaten
        const { target: iceTarget, candidate } = data.payload;
        const icePeer = connections.get(iceTarget);
        if (icePeer) {
          sendToClient(icePeer, 'ice_candidate', {
            candidate,
            peerId: clientId
          });
        }
        break;

      default:
        console.warn('Unbekannter Nachrichtentyp:', data.type);
    }
  });

  ws.on('close', () => {
    // Entferne Client aus allen Räumen
    rooms.forEach((peers, roomId) => {
      if (peers.has(clientId)) {
        peers.delete(clientId);
        if (peers.size === 0) {
          rooms.delete(roomId);
        }
        broadcastToRoom(roomId, clientId, 'peer_left', { peerId: clientId });
      }
    });
    
    // Entferne Client aus den Verbindungen
    connections.delete(clientId);
  });
});

// Express Route für Room-Liste
app.get('/api/rooms', (req, res) => {
  const roomList = Array.from(rooms.keys()).map(roomId => ({
    id: roomId,
    peers: Array.from(rooms.get(roomId)).length
  }));
  res.json(roomList);
});

// Starte Server
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Signaling Server läuft auf Port ${PORT}`);
});

// Client Beispiel-Code
/*
// Verbindung zum Signaling Server herstellen
const ws = new WebSocket('ws://localhost:3000');
let clientId;

ws.onmessage = async (event) => {
  const data = JSON.parse(event.data);

  switch (data.type) {
    case 'connected':
      clientId = data.payload.clientId;
      break;

    case 'peer_joined':
      // Neuer Peer ist dem Raum beigetreten
      const peerConnection = new RTCPeerConnection({
        iceServers: [
          { urls: 'stun:stun.l.google.com:19302' },
          // Fügen Sie hier TURN-Server hinzu
        ]
      });

      // Erstelle und sende Offer
      const offer = await peerConnection.createOffer();
      await peerConnection.setLocalDescription(offer);
      ws.send(JSON.stringify({
        type: 'offer',
        payload: {
          target: data.payload.peerId,
          sdp: offer
        }
      }));
      break;

    case 'offer':
      // Empfange Offer und sende Answer
      const pc = new RTCPeerConnection({
        iceServers: [
          { urls: 'stun:stun.l.google.com:19302' }
        ]
      });
      
      await pc.setRemoteDescription(new RTCSessionDescription(data.payload.sdp));
      const answer = await pc.createAnswer();
      await pc.setLocalDescription(answer);
      
      ws.send(JSON.stringify({
        type: 'answer',
        payload: {
          target: data.payload.peerId,
          sdp: answer
        }
      }));
      break;

    case 'answer':
      // Empfange und setze Answer
      await peerConnection.setRemoteDescription(
        new RTCSessionDescription(data.payload.sdp)
      );
      break;

    case 'ice_candidate':
      // Füge ICE Kandidaten hinzu
      if (data.payload.candidate) {
        await peerConnection.addIceCandidate(
          new RTCIceCandidate(data.payload.candidate)
        );
      }
      break;
  }
};

// ICE Kandidaten senden
peerConnection.onicecandidate = (event) => {
  if (event.candidate) {
    ws.send(JSON.stringify({
      type: 'ice_candidate',
      payload: {
        target: remotePeerId,
        candidate: event.candidate
      }
    }));
  }
};

// Einem Raum beitreten
ws.send(JSON.stringify({
  type: 'join_room',
  payload: { roomId: 'test-room' }
}));
*/