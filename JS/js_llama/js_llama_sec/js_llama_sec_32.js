// Canvas Protection
function getCanvasFingerprint() {
    const canvas = document.createElement('canvas');
    const context = canvas.getContext('2d');
  
    // Läuft die Zeit von 0 bis 100ms
    setTimeout(() => {
      // Zeichnet ein Bild auf dem Canvas
      context.fillStyle = 'rgba(255, 255, 255, 1)';
      context.fillRect(0, 0, canvas.width, canvas.height);
      context.font = '24px Arial';
      context.textAlign = 'left';
      context.textBaseline = 'top';
      context.fillText('Test', 10, 10);
  
      // Läuft die Zeit von 0 bis 100ms
      setTimeout(() => {
        // Setzt den Canvas wieder auf seine Ausgangsposition
        canvas.width = canvas.width;
        canvas.height = canvas.height;
  
        // Rückgabewert der Fingerabdruck-Nummer
        return context.getImageData(10, 10, 50, 50).data;
      }, 100);
    }, 100);
  }
  
  // Audio Protection
  function getAudioFingerprint() {
    const audioContext = new AudioContext();
    const streamSource = audioContext.createMediaStreamSource(null);
  
    // Setzt die Frequenz des Audio-Streams auf eine bestimmte Zahl
    const frequency = Math.random() * 2000 + 100;
    const oscillator = audioContext.createOscillator();
  
    oscillator.type = 'sine';
    oscillator.frequency.value = frequency;
  
    streamSource.connect(oscillator);
    oscillator.connect(audioContext.destination);
  
    // Wird das Audio-Stream auf die Echokammer zurückgegeben
    return new Promise((resolve) => {
      setTimeout(() => {
        streamSource.stop();
        oscillator.stop();
  
        // Rückgabewert der Fingerabdruck-Nummer
        resolve(oscillator.frequency.value);
      }, 1000);
    });
  }
  
  // Font Enumeration Protection
  function getFontFingerprint() {
    const fonts = ['Arial', 'Times New Roman', 'Courier New'];
  
    // Wählt eine zufällige Schrift aus
    const font = fonts[Math.floor(Math.random() * fonts.length)];
  
    // Setzt die Schrift auf das Dokument
    document.body.style.fontFamily = `${font}, sans-serif`;
  
    // Rückgabewert der Fingerabdruck-Nummer
    return font;
  }
  
  // Hardware Info Protection
  function getHardwareFingerprint() {
    const hardwareInfo = {};
  
    try {
      hardwareInfo['cpuArchitecture'] = navigator.cpuArchitecture || 'unknown';
      hardwareInfo['screenResolution'] = window.screen && window.screen.width ? 
  `${window.screen.width}x${window.screen.height}` : 'unknown';
      hardwareInfo['platform'] = navigator.platform || 'unknown';
  
      // Rückgabewert der Fingerabdruck-Nummer
      return JSON.stringify(hardwareInfo);
    } catch (error) {
      console.error(error);
  
      // Rückgabewert der Fingerabdruck-Nummer
      return 'hardware_info_error';
    }
  }
  
  // Browser API Spoofing
  function getSpoofedBrowserFingerprint() {
    const spoofedFingerprints = [
      { 'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3' },
      { 'user-agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_6) AppleWebKit/603.2.4 (KHTML,  like Gecko) Version/10.1 Mobile/14E304 Safari/603.2.4' },
    ];
  
    // Zufällig auswählt eine Fingerabdruck-Nummer
    const fingerprint = spoofedFingerprints[Math.floor(Math.random() * 
  spoofedFingerprints.length)];
  
    return fingerprint['user-agent'];
  }
  
  // Funktion zum Setzen des Browser-Fingerprinting-Protektions
  function setBrowserProtection() {
    document.body.style.fontFamily = 'Arial, sans-serif'; // Font-Enumeration-Schutz
  
    canvas.getContext('2d').fillText('Test', 10, 10); // Canvas-Schutz
  
    audioContext.createMediaStreamSource(null).start(); // Audio-Schutz
  
    window.screen && window.screen.width ? document.body.style.fontFamily = 'Arial, sans-serif' : 
  null; // Hardware Info-Schutz
  
    setInterval(() => {
      getSpoofedBrowserFingerprint();
    }, 10000); // Browser API Spoofing
  }
  
  // Funktion zum Ausführen des Browser-Fingerprinting-Protektions
  function runBrowserProtection() {
    setCanvasFingerprint(getCanvasFingerprint());
    getAudioFingerprint().then((audioFingerprint) => {
      return getFontFingerprint();
    }).then((fontFingerprint) => {
      const hardwareInfo = getHardwareFingerprint();
  
      // Rückgabewert der Fingerabdruck-Nummer
      console.log('Canvas Fingerprint:', getCanvasFingerprint());
      console.log('Audio Fingerprint:', audioFingerprint);
      console.log('Font Enumeration Fingerprint:', fontFingerprint);
      console.log('Hardware Info Fingerprint:', hardwareInfo);
      console.log('Spoofed Browser Fingerprint:', getSpoofedBrowserFingerprint());
  
      // Ausführung des Browser-Fingerprinting-Protektions
      setBrowserProtection();
    });
  }
  
  // Ausführen des Browser-Fingerprinting-Protektions
  runBrowserProtection();