class WebRTCManager {
    constructor(config = {}) {
        this.config = {
            iceServers: [
                {
                    urls: ['stun:stun1.l.google.com:19302', 'stun:stun2.l.google.com:19302']
                },
                {
                    urls: config.turnServer || 'turn:your-turn-server.com:3478',
                    username: config.turnUsername || 'username',
                    credential: config.turnPassword || 'password'
                }
            ],
            bandwidthLimits: {
                video: config.videoBandwidth || 1000, // kbps
                audio: config.audioBandwidth || 128   // kbps
            },
            mediaConstraints: {
                audio: true,
                video: {
                    width: { max: 1280 },
                    height: { max: 720 },
                    frameRate: { max: 30 }
                }
            }
        };

        this.peerConnection = null;
        this.localStream = null;
        this.remoteStream = null;
        this.iceCandidates = [];
        this.connectionState = 'new';
        
        // Event callbacks
        this.onConnectionStateChange = null;
        this.onIceCandidate = null;
        this.onTrack = null;
        this.onError = null;
    }

    // Initialize peer connection with security measures
    async initializePeerConnection() {
        try {
            // Create peer connection with ICE server configuration
            this.peerConnection = new RTCPeerConnection({
                iceServers: this.config.iceServers,
                iceTransportPolicy: 'all',
                bundlePolicy: 'max-bundle',
                rtcpMuxPolicy: 'require',
                // Enable DTLS for secure communication
                certificates: [await RTCPeerConnection.generateCertificate({
                    name: 'ECDSA',
                    namedCurve: 'P-256'
                })]
            });

            // Set up event listeners
            this.setupEventListeners();

            return this.peerConnection;
        } catch (error) {
            this.handleError('Failed to initialize peer connection', error);
            throw error;
        }
    }

    // Set up all necessary event listeners
    setupEventListeners() {
        // ICE candidate handling
        this.peerConnection.onicecandidate = (event) => {
            if (event.candidate) {
                this.handleIceCandidate(event.candidate);
            }
        };

        // Connection state monitoring
        this.peerConnection.onconnectionstatechange = () => {
            this.connectionState = this.peerConnection.connectionState;
            this.handleConnectionStateChange(this.connectionState);
        };

        // Track handling for remote streams
        this.peerConnection.ontrack = (event) => {
            this.remoteStream = event.streams[0];
            if (this.onTrack) {
                this.onTrack(event);
            }
        };

        // ICE connection state monitoring
        this.peerConnection.oniceconnectionstatechange = () => {
            console.log('ICE Connection State:', this.peerConnection.iceConnectionState);
        };
    }

    // Handle ICE candidates
    handleIceCandidate(candidate) {
        this.iceCandidates.push(candidate);
        if (this.onIceCandidate) {
            this.onIceCandidate(candidate);
        }
    }

    // Add received ICE candidate from remote peer
    async addIceCandidate(candidate) {
        try {
            await this.peerConnection.addIceCandidate(new RTCIceCandidate(candidate));
        } catch (error) {
            this.handleError('Failed to add ICE candidate', error);
        }
    }

    // Handle connection state changes
    handleConnectionStateChange(state) {
        console.log('Connection State:', state);
        if (this.onConnectionStateChange) {
            this.onConnectionStateChange(state);
        }

        // Implement automatic reconnection for failed states
        if (state === 'failed') {
            this.handleConnectionFailure();
        }
    }

    // Handle connection failures
    async handleConnectionFailure() {
        console.log('Attempting to recover from connection failure...');
        try {
            await this.restartIce();
        } catch (error) {
            this.handleError('Failed to recover connection', error);
        }
    }

    // Restart ICE connection
    async restartIce() {
        try {
            const offer = await this.peerConnection.createOffer({ iceRestart: true });
            await this.peerConnection.setLocalDescription(offer);
        } catch (error) {
            this.handleError('Failed to restart ICE', error);
        }
    }

    // Set up local media stream with security checks
    async setupLocalStream() {
        try {
            // Request media with constraints
            this.localStream = await navigator.mediaDevices.getUserMedia(
                this.config.mediaConstraints
            );

            // Add tracks to peer connection
            this.localStream.getTracks().forEach(track => {
                this.peerConnection.addTrack(track, this.localStream);
            });

            // Apply bandwidth limits
            this.applyBandwidthLimits();

            return this.localStream;
        } catch (error) {
            this.handleError('Failed to setup local stream', error);
            throw error;
        }
    }

    // Apply bandwidth limitations
    applyBandwidthLimits() {
        const senders = this.peerConnection.getSenders();
        senders.forEach(sender => {
            if (sender.track) {
                const params = sender.getParameters();
                if (!params.encodings) {
                    params.encodings = [{}];
                }

                // Set maximum bitrate based on track type
                if (sender.track.kind === 'video') {
                    params.encodings[0].maxBitrate = this.config.bandwidthLimits.video * 1000;
                } else if (sender.track.kind === 'audio') {
                    params.encodings[0].maxBitrate = this.config.bandwidthLimits.audio * 1000;
                }

                sender.setParameters(params).catch(error => {
                    this.handleError('Failed to set bandwidth limits', error);
                });
            }
        });
    }

    // Create and send offer
    async createOffer() {
        try {
            const offer = await this.peerConnection.createOffer();
            await this.peerConnection.setLocalDescription(offer);
            return offer;
        } catch (error) {
            this.handleError('Failed to create offer', error);
            throw error;
        }
    }

    // Handle received offer
    async handleOffer(offer) {
        try {
            await this.peerConnection.setRemoteDescription(new RTCSessionDescription(offer));
            const answer = await this.peerConnection.createAnswer();
            await this.peerConnection.setLocalDescription(answer);
            return answer;
        } catch (error) {
            this.handleError('Failed to handle offer', error);
            throw error;
        }
    }

    // Handle received answer
    async handleAnswer(answer) {
        try {
            await this.peerConnection.setRemoteDescription(new RTCSessionDescription(answer));
        } catch (error) {
            this.handleError('Failed to handle answer', error);
            throw error;
        }
    }

    // Error handling
    handleError(message, error) {
        console.error(message, error);
        if (this.onError) {
            this.onError({ message, error });
        }
    }

    // Clean up resources
    cleanup() {
        if (this.localStream) {
            this.localStream.getTracks().forEach(track => track.stop());
        }
        
        if (this.peerConnection) {
            this.peerConnection.close();
        }

        this.localStream = null;
        this.remoteStream = null;
        this.peerConnection = null;
        this.iceCandidates = [];
    }
}
