class P2PSystem {
    constructor(config = {}) {
        this.peers = new Map(); // Active peer connections
        this.config = {
            maxPeers: config.maxPeers || 10,
            maxDataRate: config.maxDataRate || 1000000, // 1 MB/s
            heartbeatInterval: config.heartbeatInterval || 30000, // 30 seconds
            iceServers: config.iceServers || [
                { urls: 'stun:stun.l.google.com:19302' }
            ]
        };
        
        // Rate Limiting
        this.dataCounter = 0;
        this.lastResetTime = Date.now();
        
        // Security
        this.securityKey = crypto.getRandomValues(new Uint8Array(32));
        
        // Event Handlers
        this.onPeerConnected = null;
        this.onPeerDisconnected = null;
        this.onMessage = null;
    }

    async connectToPeer(peerId, signal) {
        if (this.peers.size >= this.config.maxPeers) {
            throw new Error('Maximum number of peers reached');
        }

        const peerConnection = new RTCPeerConnection({
            iceServers: this.config.iceServers
        });

        const dataChannel = peerConnection.createDataChannel('p2p-channel', {
            ordered: true
        });

        this.setupDataChannel(dataChannel, peerId);

        peerConnection.onicecandidate = (event) => {
            if (event.candidate) {
                this.signalPeer(peerId, {
                    type: 'ice-candidate',
                    candidate: event.candidate
                });
            }
        };

        this.peers.set(peerId, {
            connection: peerConnection,
            dataChannel: dataChannel,
            lastHeartbeat: Date.now()
        });

        if (signal) {
            const remoteDesc = new RTCSessionDescription(signal);
            await peerConnection.setRemoteDescription(remoteDesc);
            const answer = await peerConnection.createAnswer();
            await peerConnection.setLocalDescription(answer);
            return answer;
        } else {
            const offer = await peerConnection.createOffer();
            await peerConnection.setLocalDescription(offer);
            return offer;
        }
    }

    setupDataChannel(dataChannel, peerId) {
        dataChannel.onopen = () => {
            console.log(`Data channel opened to peer ${peerId}`);
            this.startHeartbeat(peerId);
            if (this.onPeerConnected) {
                this.onPeerConnected(peerId);
            }
        };

        dataChannel.onclose = () => {
            console.log(`Data channel closed to peer ${peerId}`);
            this.peers.delete(peerId);
            if (this.onPeerDisconnected) {
                this.onPeerDisconnected(peerId);
            }
        };

        dataChannel.onmessage = (event) => {
            this.handleMessage(peerId, event.data);
        };
    }

    async sendToPeer(peerId, data) {
        const peer = this.peers.get(peerId);
        if (!peer || peer.dataChannel.readyState !== 'open') {
            throw new Error('Peer not connected');
        }

        if (!this.checkRateLimit(data.length)) {
            throw new Error('Rate limit exceeded');
        }

        const encryptedData = await this.encryptData(data);
        
        peer.dataChannel.send(encryptedData);
    }

    checkRateLimit(dataSize) {
        const now = Date.now();
        if (now - this.lastResetTime > 1000) {
            this.dataCounter = 0;
            this.lastResetTime = now;
        }

        this.dataCounter += dataSize;
        return this.dataCounter <= this.config.maxDataRate;
    }

    async encryptData(data) {
        const encoder = new TextEncoder();
        const dataBuffer = encoder.encode(data);
        
        const key = await crypto.subtle.importKey(
            'raw',
            this.securityKey,
            { name: 'AES-GCM' },
            false,
            ['encrypt']
        );

        const iv = crypto.getRandomValues(new Uint8Array(12));
        const encryptedData = await crypto.subtle.encrypt(
            {
                name: 'AES-GCM',
                iv: iv
            },
            key,
            dataBuffer
        );

        const resultBuffer = new Uint8Array(iv.length + encryptedData.byteLength);
        resultBuffer.set(iv);
        resultBuffer.set(new Uint8Array(encryptedData), iv.length);
        
        return resultBuffer;
    }

    async decryptData(encryptedData) {
        const iv = encryptedData.slice(0, 12);
        const data = encryptedData.slice(12);

        const key = await crypto.subtle.importKey(
            'raw',
            this.securityKey,
            { name: 'AES-GCM' },
            false,
            ['decrypt']
        );

        const decryptedData = await crypto.subtle.decrypt(
            {
                name: 'AES-GCM',
                iv: iv
            },
            key,
            data
        );

        const decoder = new TextDecoder();
        return decoder.decode(decryptedData);
    }

    startHeartbeat(peerId) {
        setInterval(() => {
            const peer = this.peers.get(peerId);
            if (peer && peer.dataChannel.readyState === 'open') {
                this.sendToPeer(peerId, JSON.stringify({
                    type: 'heartbeat',
                    timestamp: Date.now()
                }));
            }
        }, this.config.heartbeatInterval);
    }

    async handleMessage(peerId, encryptedData) {
        try {
            const decryptedData = await this.decryptData(encryptedData);
            const message = JSON.parse(decryptedData);

            if (message.type === 'heartbeat') {
                const peer = this.peers.get(peerId);
                if (peer) {
                    peer.lastHeartbeat = message.timestamp;
                }
                return;
            }

            if (this.onMessage) {
                this.onMessage(peerId, message);
            }
        } catch (error) {
            console.error('Error handling message:', error);
        }
    }

    cleanup() {
        for (const [peerId, peer] of this.peers.entries()) {
            const timeSinceLastHeartbeat = Date.now() - peer.lastHeartbeat;
            if (timeSinceLastHeartbeat > this.config.heartbeatInterval * 2) {
                console.log(`Peer ${peerId} timeout - closing connection`);
                peer.connection.close();
                this.peers.delete(peerId);
            }
        }
    }
}

const initP2P = async () => {
    const p2p = new P2PSystem({
        maxPeers: 5,
        maxDataRate: 500000, // 500 KB/s
        heartbeatInterval: 15000 // 15 seconds
    });

    p2p.onPeerConnected = (peerId) => {
        console.log(`New peer connection: ${peerId}`);
    };

    p2p.onPeerDisconnected = (peerId) => {
        console.log(`Peer connection disconnected: ${peerId}`);
    };

    p2p.onMessage = (peerId, message) => {
        console.log(`Message from ${peerId}:`, message);
    };

    setInterval(() => {
        p2p.cleanup();
    }, 60000); // Every minute

    return p2p;
};