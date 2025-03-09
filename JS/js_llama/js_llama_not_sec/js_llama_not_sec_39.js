const WebSocket = require('ws');
const wss = new WebSocket.Server({ port: 8080 });

// Create a map to store all clients
let clients = {};

// Client-Verbindung-Handler
wss.on('connection', (ws) => {
    ws.on('message', (data) => {
        // Process the incoming message
        processMessage(ws, data);
    });
});

function processMessage(ws, data) {
    // Check if the client is already connected
    if (clients[ws.id]) {
        return;
    }

    // Add the client to the map
    clients[ws.id] = true;

    // Check if the message is a discovery request
    if (data.type === 'discovery') {
        // Create an IceServer for NAT traversal
        let iceServer = createIceServer();
        ws.send(JSON.stringify({ type: 'ice-server', data: iceServer }));
    }

    // Check if the message is a connection request
    else if (data.type === 'connect') {
        // Connect the client to another client
        connectClients(ws, data.clientId);
    }
}

// Function to create an IceServer for NAT traversal
function createIceServer() {
    return {
        "urls": [
            "stun:192.0.2.1:3478",
            "turn:192.0.2.15:5000?user=client&pass=123456",
            "turn:192.0.2.16:3479?user=server&pass=hello"
        ],
    };
}

// Function to connect two clients
function connectClients(ws, clientId) {
    // Find the client with the matching ID
    let targetClient = Object.keys(clients).find((id) => id === clientId);

    if (targetClient) {
        // Create a new connection between the two clients
        createConnection(ws, targetClient);
    }
}

// Function to create a new connection between two clients
function createConnection(ws, clientId) {
    // Find the client with the matching ID
    let targetClient = Object.keys(clients).find((id) => id === clientId);

    if (targetClient) {
        // Create a new WebSocket connection between the two clients
        let targetWs = wss.clients.find((client) => client.id === targetClient);

        // Establish the peer connection
        let pc = new RTCPeerConnection();

        // Add the ICE candidate to the remote description
        targetWs.send(JSON.stringify({ type: 'ice-candidate', data: targetWs.data }));

        // Start the offer creation process
        pc.createOffer().then((offer) => {
            return pc.setLocalDescription(new RTCSessionDescription({ type: 'offer', sdp: offer 
}));
        }).catch((error) => {
            console.error('Error creating offer:', error);
        });

        // Wait for the answer from the target client
        targetWs.onmessage = (event) => {
            if (event.data.type === 'answer') {
                return pc.setRemoteDescription(new RTCSessionDescription({ type: 'answer', sdp: 
event.data.data }));
            }
        };
    }
}
