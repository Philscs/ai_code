import path from 'path';
import fs from 'fs/promises';
import esbuild from 'esbuild';
import { createServer } from 'http';
import WebSocket from 'ws';

class Bundler {
  constructor(entry, output) {
    this.entry = entry;
    this.output = output;
    this.dependencies = new Set();
    this.clients = [];
  }

  async bundle() {
    console.log('Bundling...');
    try {
      const result = await esbuild.build({
        entryPoints: [this.entry],
        bundle: true,
        splitting: true,
        format: 'esm',
        outdir: path.dirname(this.output),
        sourcemap: true,
        treeShaking: true,
        watch: {
          onRebuild: (error, result) => {
            if (error) {
              console.error('Rebuild failed:', error);
            } else {
              console.log('Rebuilt successfully');
              this.broadcastUpdate();
            }
          },
        },
      });
      console.log('Bundle completed:', result);
    } catch (error) {
      console.error('Bundle error:', error);
    }
  }

  startHMRServer(port = 3000) {
    const server = createServer();
    const wss = new WebSocket.Server({ server });

    wss.on('connection', (ws) => {
      console.log('Client connected');
      this.clients.push(ws);
      ws.on('close', () => {
        console.log('Client disconnected');
        this.clients = this.clients.filter((client) => client !== ws);
      });
    });

    server.listen(port, () => {
      console.log(`HMR Server running on http://localhost:${port}`);
    });
  }

  broadcastUpdate() {
    console.log('Broadcasting update to clients...');
    this.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send('update');
      }
    });
  }
}

// Example Usage
const entry = path.resolve('src/index.js');
const output = path.resolve('dist/bundle.js');

const bundler = new Bundler(entry, output);
bundler.bundle();
bundler.startHMRServer();
