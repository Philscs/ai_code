// Prevent Canvas Fingerprinting
(function protectCanvas() {
    const originalToDataURL = HTMLCanvasElement.prototype.toDataURL;
    HTMLCanvasElement.prototype.toDataURL = function (...args) {
        const ctx = this.getContext('2d');
        if (ctx) {
            ctx.fillStyle = 'rgba(255, 255, 255, 0.5)';
            ctx.fillRect(0, 0, this.width, this.height);
        }
        return originalToDataURL.apply(this, args);
    };

    const originalGetImageData = CanvasRenderingContext2D.prototype.getImageData;
    CanvasRenderingContext2D.prototype.getImageData = function (...args) {
        const imageData = originalGetImageData.apply(this, args);
        for (let i = 0; i < imageData.data.length; i += 4) {
            imageData.data[i] ^= 255; // Obfuscate color channels
            imageData.data[i + 1] ^= 255;
            imageData.data[i + 2] ^= 255;
        }
        return imageData;
    };
})();

// Prevent Audio Fingerprinting
(function protectAudio() {
    const originalOscillatorStart = OscillatorNode.prototype.start;
    OscillatorNode.prototype.start = function (...args) {
        const gainNode = this.context.createGain();
        gainNode.gain.value = 0.5; // Modify audio gain to obfuscate signature
        this.connect(gainNode).connect(this.context.destination);
        originalOscillatorStart.apply(this, args);
    };
})();

// Prevent Font Enumeration
(function protectFonts() {
    const originalFonts = Object.getOwnPropertyDescriptor(HTMLElement.prototype, 'offsetWidth');
    Object.defineProperty(HTMLElement.prototype, 'offsetWidth', {
        get: function () {
            return 42; // Return a fixed width for all fonts
        }
    });
    Object.defineProperty(HTMLElement.prototype, 'offsetHeight', {
        get: function () {
            return 42; // Return a fixed height for all fonts
        }
    });
})();

// Protect Hardware Info
(function protectHardware() {
    Object.defineProperty(navigator, 'hardwareConcurrency', {
        get: () => 4 // Return a fixed number of logical processors
    });
    Object.defineProperty(navigator, 'deviceMemory', {
        get: () => 4 // Return a fixed amount of memory
    });
})();

// Spoof Browser APIs
(function spoofBrowserAPI() {
    const originalUserAgent = navigator.userAgent;
    Object.defineProperty(navigator, 'userAgent', {
        get: () => 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
    });

    Object.defineProperty(navigator, 'platform', {
        get: () => 'Win32'
    });

    Object.defineProperty(navigator, 'languages', {
        get: () => ['en-US', 'en']
    });
})();
