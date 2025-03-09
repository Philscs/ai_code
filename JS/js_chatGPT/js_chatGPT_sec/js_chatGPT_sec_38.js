// Simple P2P System with WebRTC Data Channels

// Dependencies: Include signaling server (like WebSocket) for peer discovery
const signalingServerUrl = 'wss://your-signaling-server.com'; // Replace with your signaling server
const peers = new Map(); // Store connected peers
const localConnection = new RTCPeerConnection();

// Signaling WebSocket for peer discovery and signaling
const signalingSocket = new WebSocket(signalingServerUrl);

signalingSocket.onmessage = async (message) => {
  const data = JSON.parse(message.data);
  if (data.offer) {
    await handleOffer(data.offer, data.from);
  } else if (data.answer) {
    await handleAnswer(data.answer, data.from);
  } else if (data.candidate) {
    await handleCandidate(data.candidate, data.from);
  }
};

// Security Controls: Restrict channels and data handling
const allowedChannels = new Set(['chat', 'file-transfer']);

function isAllowedChannel(channelName) {
  return allowedChannels.has(channelName);
}

// Create a Data Channel for communication
function createDataChannel(label) {
  if (!isAllowedChannel(label)) {
    throw new Error('Unauthorized channel creation');
  }

  const channel = localConnection.createDataChannel(label);
  channel.onmessage = (event) => {
    console.log(`Message from ${label}:`, event.data);
    // Rate Limiting Example: Basic control to throttle messages
    // Implement actual logic as needed
  };

  channel.onerror = (error) => console.error('Channel error:', error);
  return channel;
}

// Handle offers from peers
async function handleOffer(offer, peerId) {
  const remoteConnection = new RTCPeerConnection();
  peers.set(peerId, remoteConnection);
  await remoteConnection.setRemoteDescription(new RTCSessionDescription(offer));

  const answer = await remoteConnection.createAnswer();
  await remoteConnection.setLocalDescription(answer);

  signalingSocket.send(JSON.stringify({
    answer,
    to: peerId,
  }));

  remoteConnection.ondatachannel = (event) => {
    const channel = event.channel;
    if (isAllowedChannel(channel.label)) {
      channel.onmessage = (e) => console.log('Data received:', e.data);
    } else {
      console.warn('Unauthorized data channel attempted');
    }
  };
}

// Handle answers from peers
async function handleAnswer(answer, peerId) {
  const peer = peers.get(peerId);
  if (peer) {
    await peer.setRemoteDescription(new RTCSessionDescription(answer));
  }
}

// Handle ICE candidates
async function handleCandidate(candidate, peerId) {
  const peer = peers.get(peerId);
  if (peer) {
    await peer.addIceCandidate(new RTCIceCandidate(candidate));
  }
}

// Resource Management: Close connections and cleanup
function closeConnection(peerId) {
  const peer = peers.get(peerId);
  if (peer) {
    peer.close();
    peers.delete(peerId);
  }
}

// Connect to a new peer
async function connectToPeer(peerId) {
  const connection = new RTCPeerConnection();
  peers.set(peerId, connection);

  const channel = createDataChannel('chat'); // Example channel

  connection.onicecandidate = (event) => {
    if (event.candidate) {
      signalingSocket.send(JSON.stringify({
        candidate: event.candidate,
        to: peerId,
      }));
    }
  };

  const offer = await connection.createOffer();
  await connection.setLocalDescription(offer);

  signalingSocket.send(JSON.stringify({
    offer,
    to: peerId,
  }));
}

// Example: Initiate connection to a peer
// connectToPeer('peer-id');
