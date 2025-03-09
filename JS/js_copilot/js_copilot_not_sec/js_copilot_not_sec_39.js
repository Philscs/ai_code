
const express = require('express');
const app = express();
const http = require('http').createServer(app);
const io = require('socket.io')(http);

// Peer Discovery
const peers = {};

io.on('connection', (socket) => {
  // Connection Management
  socket.on('join', (roomId, userId) => {
    socket.join(roomId);
    socket.userId = userId;
    peers[userId] = socket.id;
    io.to(roomId).emit('userJoined', userId);
  });

  socket.on('disconnect', () => {
    const userId = socket.userId;
    delete peers[userId];
    io.emit('userLeft', userId);
  });

  // Message Relay
  socket.on('message', (roomId, userId, message) => {
    io.to(roomId).emit('message', userId, message);
  });

  // Room Management
  socket.on('createRoom', (roomId) => {
    socket.join(roomId);
    socket.emit('roomCreated', roomId);
  });

  socket.on('joinRoom', (roomId) => {
    socket.join(roomId);
    socket.emit('roomJoined', roomId);
  });
});

// NAT Traversal
const port = process.env.PORT || 3000;
http.listen(port, () => {
  console.log(`Server is running on port ${port}`);
});
