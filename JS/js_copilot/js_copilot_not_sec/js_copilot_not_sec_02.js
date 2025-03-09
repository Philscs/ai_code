
const WebSocket = require('ws');

// Create a WebSocket server
const wss = new WebSocket.Server({ port: 8080 });

// Store the whiteboard state
let whiteboardState = {
  lines: [],
  undoStack: [],
};

// Broadcast the whiteboard state to all connected clients
function broadcastState() {
  const state = JSON.stringify(whiteboardState);
  wss.clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(state);
    }
  });
}

// Handle incoming WebSocket connections
wss.on('connection', ws => {
  // Send the current whiteboard state to the new client
  ws.send(JSON.stringify(whiteboardState));

  // Handle incoming messages from clients
  ws.on('message', message => {
    const data = JSON.parse(message);

    // Handle different types of messages
    switch (data.type) {
      case 'draw':
        // Add the new line to the whiteboard state
        whiteboardState.lines.push(data.line);
        // Clear the undo stack
        whiteboardState.undoStack = [];
        // Broadcast the updated state to all clients
        broadcastState();
        break;
      case 'undo':
        // Check if there are lines to undo
        if (whiteboardState.lines.length > 0) {
          // Remove the last line from the whiteboard state
          const undoneLine = whiteboardState.lines.pop();
          // Add the undone line to the undo stack
          whiteboardState.undoStack.push(undoneLine);
          // Broadcast the updated state to all clients
          broadcastState();
        }
        break;
      case 'redo':
        // Check if there are lines to redo
        if (whiteboardState.undoStack.length > 0) {
          // Remove the last line from the undo stack
          const redoneLine = whiteboardState.undoStack.pop();
          // Add the redone line back to the whiteboard state
          whiteboardState.lines.push(redoneLine);
          // Broadcast the updated state to all clients
          broadcastState();
        }
        break;
      default:
        console.log('Unknown message type:', data.type);
    }
  });
});
