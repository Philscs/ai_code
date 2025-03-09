class P2PSystem {
    constructor(config = {}) {
        this.peers = new Map(); // Aktive Peer-Verbindungen
        this.config = {
            maxPeers: config.maxPeers || 10,
            maxDataRate: config.maxDataRate || 1000000, // 1 MB/s
            heartbeatInterval: config.heartbeatInterval || 30000, // 30 Sekunden
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

    // Peer Discovery und Verbindungsaufbau
    async connectToPeer(peerId, signal) {
        if (this.peers.size >= this.config.maxPeers) {
            throw new Error('Maximale Anzahl an Peers erreicht');
        }

        const peerConnection = new RTCPeerConnection({
            iceServers: this.config.iceServers
        });

        // Datenkanal erstellen
        const dataChannel = peerConnection.createDataChannel('p2p-channel', {
            ordered: true
        });

        // Event Listener für den Datenkanal
        this.setupDataChannel(dataChannel, peerId);

        // ICE Kandidaten Handler
        peerConnection.onicecandidate = (event) => {
            if (event.candidate) {
                // Signal an den Remote Peer senden
                this.signalPeer(peerId, {
                    type: 'ice-candidate',
                    candidate: event.candidate
                });
            }
        };

        // Verbindung in der Map speichern
        this.peers.set(peerId, {
            connection: peerConnection,
            dataChannel: dataChannel,
            lastHeartbeat: Date.now()
        });

        // Verbindungsaufbau starten
        if (signal) {
            // Answer erstellen
            const remoteDesc = new RTCSessionDescription(signal);
            await peerConnection.setRemoteDescription(remoteDesc);
            const answer = await peerConnection.createAnswer();
            await peerConnection.setLocalDescription(answer);
            return answer;
        } else {
            // Offer erstellen
            const offer = await peerConnection.createOffer();
            await peerConnection.setLocalDescription(offer);
            return offer;
        }
    }

    // Datenkanal Setup
    setupDataChannel(dataChannel, peerId) {
        dataChannel.onopen = () => {
            console.log(`Datenkanal zu Peer ${peerId} geöffnet`);
            this.startHeartbeat(peerId);
            if (this.onPeerConnected) {
                this.onPeerConnected(peerId);
            }
        };

        dataChannel.onclose = () => {
            console.log(`Datenkanal zu Peer ${peerId} geschlossen`);
            this.peers.delete(peerId);
            if (this.onPeerDisconnected) {
                this.onPeerDisconnected(peerId);
            }
        };

        dataChannel.onmessage = (event) => {
            this.handleMessage(peerId, event.data);
        };
    }

    // Nachrichten-Handling mit Rate Limiting
    async sendToPeer(peerId, data) {
        const peer = this.peers.get(peerId);
        if (!peer || peer.dataChannel.readyState !== 'open') {
            throw new Error('Peer nicht verbunden');
        }

        // Rate Limiting prüfen
        if (!this.checkRateLimit(data.length)) {
            throw new Error('Rate Limit überschritten');
        }

        // Daten verschlüsseln
        const encryptedData = await this.encryptData(data);
        
        peer.dataChannel.send(encryptedData);
    }

    // Rate Limiting Implementation
    checkRateLimit(dataSize) {
        const now = Date.now();
        if (now - this.lastResetTime > 1000) {
            this.dataCounter = 0;
            this.lastResetTime = now;
        }

        this.dataCounter += dataSize;
        return this.dataCounter <= this.config.maxDataRate;
    }

    // Verschlüsselung der Daten
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

        // IV und verschlüsselte Daten zusammenführen
        const resultBuffer = new Uint8Array(iv.length + encryptedData.byteLength);
        resultBuffer.set(iv);
        resultBuffer.set(new Uint8Array(encryptedData), iv.length);
        
        return resultBuffer;
    }

    // Entschlüsselung der Daten
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

    // Heartbeat Mechanismus
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

    // Nachrichtenverarbeitung
    async handleMessage(peerId, encryptedData) {
        try {
            const decryptedData = await this.decryptData(encryptedData);
            const message = JSON.parse(decryptedData);

            // Heartbeat Handling
            if (message.type === 'heartbeat') {
                const peer = this.peers.get(peerId);
                if (peer) {
                    peer.lastHeartbeat = message.timestamp;
                }
                return;
            }

            // Benutzerdefinierte Nachrichtenverarbeitung
            if (this.onMessage) {
                this.onMessage(peerId, message);
            }
        } catch (error) {
            console.error('Fehler bei der Nachrichtenverarbeitung:', error);
        }
    }

    // Ressourcen-Management
    cleanup() {
        // Alte Verbindungen bereinigen
        for (const [peerId, peer] of this.peers.entries()) {
            const timeSinceLastHeartbeat = Date.now() - peer.lastHeartbeat;
            if (timeSinceLastHeartbeat > this.config.heartbeatInterval * 2) {
                console.log(`Peer ${peerId} timeout - Verbindung wird geschlossen`);
                peer.connection.close();
                this.peers.delete(peerId);
            }
        }
    }
}

// Beispiel Verwendung:
const initP2P = async () => {
    const p2p = new P2PSystem({
        maxPeers: 5,
        maxDataRate: 500000, // 500 KB/s
        heartbeatInterval: 15000 // 15 Sekunden
    });

    // Event Handler
    p2p.onPeerConnected = (peerId) => {
        console.log(`Neue Peer-Verbindung: ${peerId}`);
    };

    p2p.onPeerDisconnected = (peerId) => {
        console.log(`Peer-Verbindung getrennt: ${peerId}`);
    };

    p2p.onMessage = (peerId, message) => {
        console.log(`Nachricht von ${peerId}:`, message);
    };

    // Regelmäßige Ressourcen-Bereinigung
    setInterval(() => {
        p2p.cleanup();
    }, 60000); // Jede Minute

    return p2p;
};