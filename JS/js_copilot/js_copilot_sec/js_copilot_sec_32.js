class FingerprintProtection {
    constructor() {
        this.originalCanvas = null;
        this.originalAudio = null;
        this.originalFontList = null;
        
        // Store original functions
        this.saveOriginalFunctions();
        
        // Initialize all protections
        this.initializeCanvasProtection();
        this.initializeAudioProtection();
        this.initializeFontProtection();
        this.initializeHardwareProtection();
        this.initializeBrowserAPISpoof();
    }

    saveOriginalFunctions() {
        // Canvas
        this.originalCanvas = {
            getContext: HTMLCanvasElement.prototype.getContext,
            toDataURL: HTMLCanvasElement.prototype.toDataURL,
            toBlob: HTMLCanvasElement.prototype.toBlob
        };

        // Audio
        this.originalAudio = {
            createAnalyser: AudioContext.prototype.createAnalyser,
            createOscillator: AudioContext.prototype.createOscillator
        };

        // Font
        this.originalFontList = window.queryLocalFonts;
    }

    // Canvas Fingerprint Protection
    initializeCanvasProtection() {
        const addNoise = (imageData) => {
            const noise = Math.random() * 0.01; // Small random noise
            for (let i = 0; i < imageData.data.length; i += 4) {
                imageData.data[i] = Math.floor(imageData.data[i] * (1 + noise));
                imageData.data[i + 1] = Math.floor(imageData.data[i + 1] * (1 + noise));
                imageData.data[i + 2] = Math.floor(imageData.data[i + 2] * (1 + noise));
            }
            return imageData;
        };

        const originalToDataURL = HTMLCanvasElement.prototype.toDataURL;
        HTMLCanvasElement.prototype.toDataURL = function() {
            const context = this.getContext('2d');
            const imageData = context.getImageData(0, 0, this.width, this.height);
            const noisyImageData = addNoise(imageData);
            context.putImageData(noisyImageData, 0, 0);
            return originalToDataURL.apply(this, arguments);
        };

        const originalToBlob = HTMLCanvasElement.prototype.toBlob;
        HTMLCanvasElement.prototype.toBlob = function(callback) {
            const context = this.getContext('2d');
            const imageData = context.getImageData(0, 0, this.width, this.height);
            const noisyImageData = addNoise(imageData);
            context.putImageData(noisyImageData, 0, 0);
            return originalToBlob.apply(this, arguments);
        };
    }

    // Audio Fingerprint Protection
    initializeAudioProtection() {
        const originalCreateAnalyser = AudioContext.prototype.createAnalyser;
        AudioContext.prototype.createAnalyser = function() {
            const analyser = originalCreateAnalyser.apply(this, arguments);
            const originalGetFloatFrequencyData = analyser.getFloatFrequencyData;
            
            analyser.getFloatFrequencyData = function(array) {
                originalGetFloatFrequencyData.call(this, array);
                // Add slight random variations to frequency data
                for (let i = 0; i < array.length; i++) {
                    array[i] += (Math.random() - 0.5) * 0.1;
                }
            };
            
            return analyser;
        };

        const originalCreateOscillator = AudioContext.prototype.createOscillator;
        AudioContext.prototype.createOscillator = function() {
            const oscillator = originalCreateOscillator.apply(this, arguments);
            const originalFrequency = oscillator.frequency.value;
            
            // Add slight detuning
            oscillator.frequency.value = originalFrequency + (Math.random() - 0.5) * 0.1;
            
            return oscillator;
        };
    }

    // Font Enumeration Protection
    initializeFontProtection() {
        const standardFonts = [
            'Arial', 'Times New Roman', 'Courier New', 
            'Georgia', 'Verdana', 'Helvetica'
        ];

        window.queryLocalFonts = async function() {
            // Return only standard fonts
            return standardFonts.map(font => ({
                family: font,
                fullName: font,
                postScriptName: font.toLowerCase().replace(/\s+/g, '-'),
                style: 'Regular'
            }));
        };
    }

    // Hardware Info Protection
    initializeHardwareProtection() {
        // Override navigator.hardwareConcurrency
        Object.defineProperty(navigator, 'hardwareConcurrency', {
            get: () => 4 // Return a common value
        });

        // Override navigator.deviceMemory
        Object.defineProperty(navigator, 'deviceMemory', {
            get: () => 8 // Return a common value
        });

        // Override navigator.platform
        Object.defineProperty(navigator, 'platform', {
            get: () => 'Win32' // Return a common value
        });
    }

    // Browser API Spoofing
    initializeBrowserAPISpoof() {
        // Spoof common browser features
        const spoofedFeatures = {
            userAgent: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
            language: 'en-US',
            languages: ['en-US', 'en'],
            maxTouchPoints: 0,
            vendor: 'Google Inc.',
        };

        // Override navigator properties
        for (const [key, value] of Object.entries(spoofedFeatures)) {
            try {
                Object.defineProperty(navigator, key, {
                    get: () => value
                });
            } catch (e) {
                console.warn(`Failed to override navigator.${key}`);
            }
        }

        // Override screen properties
        const spoofedScreen = {
            width: 1920,
            height: 1080,
            colorDepth: 24,
            pixelDepth: 24
        };

        for (const [key, value] of Object.entries(spoofedScreen)) {
            try {
                Object.defineProperty(screen, key, {
                    get: () => value
                });
            } catch (e) {
                console.warn(`Failed to override screen.${key}`);
            }
        }
    }

    // Method to disable all protections and restore original behavior
    disable() {
        // Restore Canvas functions
        HTMLCanvasElement.prototype.getContext = this.originalCanvas.getContext;
        HTMLCanvasElement.prototype.toDataURL = this.originalCanvas.toDataURL;
        HTMLCanvasElement.prototype.toBlob = this.originalCanvas.toBlob;

        // Restore Audio functions
        AudioContext.prototype.createAnalyser = this.originalAudio.createAnalyser;
        AudioContext.prototype.createOscillator = this.originalAudio.createOscillator;

        // Restore Font enumeration
        window.queryLocalFonts = this.originalFontList;
    }
}

// Usage example:
const protection = new FingerprintProtection();

// To disable protections:
// protection.disable();