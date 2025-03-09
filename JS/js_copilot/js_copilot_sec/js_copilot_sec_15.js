class WebRTCConnection {
    constructor(config) {
        this.peerConnection = new RTCPeerConnection(config);
        this.remoteStream = new MediaStream();
        this.localStream = null;
        this.iceCandidatesQueue = [];

        this.setupEventHandlers();
    }

    setupEventHandlers() {
        this.peerConnection.onicecandidate = (event) => {
            if (event.candidate) {
                this.handleIceCandidate(event.candidate);
            }
        };

        this.peerConnection.ontrack = (event) => {
            this.remoteStream.addTrack(event.track);
        };

        this.peerConnection.onconnectionstatechange = () => {
            console.log('Connection state:', this.peerConnection.connectionState);
        };

        this.peerConnection.oniceconnectionstatechange = () => {
            console.log('ICE connection state:', this.peerConnection.iceConnectionState);
        };
    }

    handleIceCandidate(candidate) {
        console.log('ICE candidate:', candidate);
        this.sendToSignalingServer({ type: 'ice-candidate', candidate });
    }

    async setupLocalStream() {
        try {
            this.localStream = await navigator.mediaDevices.getUserMedia({
                video: true,
                audio: true
            });
            this.localStream.getTracks().forEach(track => {
                this.peerConnection.addTrack(track, this.localStream);
            });
        } catch (error) {
            console.error('Error accessing local media:', error);
        }
    }

    addIceCandidate(candidate) {
        if (candidate) {
            this.peerConnection.addIceCandidate(new RTCIceCandidate(candidate))
                .catch(error => console.error('Error adding ICE candidate:', error));
        }
    }

    async createOffer() {
        const offer = await this.peerConnection.createOffer();
        await this.peerConnection.setLocalDescription(offer);
        this.sendToSignalingServer({ type: 'offer', offer });
    }

    async createAnswer(offer) {
        await this.peerConnection.setRemoteDescription(new RTCSessionDescription(offer));
        const answer = await this.peerConnection.createAnswer();
        await this.peerConnection.setLocalDescription(answer);
        this.sendToSignalingServer({ type: 'answer', answer });
    }

    async handleOffer(offer) {
        await this.createAnswer(offer);
    }

    async handleAnswer(answer) {
        await this.peerConnection.setRemoteDescription(new RTCSessionDescription(answer));
    }

    setBandwidth(maxBitrate) {
        const senders = this.peerConnection.getSenders();
        senders.forEach(sender => {
            const params = sender.getParameters();
            if (!params.encodings) {
                params.encodings = [{}];
            }
            params.encodings[0].maxBitrate = maxBitrate;
            sender.setParameters(params).catch(error => console.error('Error setting bandwidth:', error));
        });
    }

    static getDefaultConfig() {
        return {
            iceServers: [
                { urls: 'stun:stun.l.google.com:19302' },
                {
                    urls: 'turn:turn.example.com',
                    username: 'user',
                    credential: 'password'
                }
            ]
        };
    }

    sendToSignalingServer(data) {
        console.log('Sending to signaling server:', data);
    }
}

(async () => {
    const config = WebRTCConnection.getDefaultConfig();
    const connection = new WebRTCConnection(config);

    await connection.setupLocalStream();
    connection.createOffer();
})();
