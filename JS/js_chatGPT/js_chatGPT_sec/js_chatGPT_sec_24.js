// Hauptdatei: audio-processor.js

class CustomAudioProcessor extends AudioWorkletProcessor {
    constructor() {
      super();
      this.buffer = new Float32Array(1024); // Speicherverwaltung: fixierte Buffer-Größe
      this.resourceLimit = 512; // Ressourcenbeschränkung: Anzahl Samples
    }
  
    process(inputs, outputs, parameters) {
      const input = inputs[0];
      const output = outputs[0];
  
      if (!input || !output) {
        return true;
      }
  
      for (let channel = 0; channel < input.length; channel++) {
        const inputChannel = input[channel];
        const outputChannel = output[channel];
  
        for (let i = 0; i < inputChannel.length; i++) {
          // Sicherheitskontrolle: Werte validieren
          const sample = inputChannel[i];
          if (Math.abs(sample) > 1) {
            outputChannel[i] = 0; // Werte außerhalb des Bereichs abschneiden
          } else {
            outputChannel[i] = sample * 0.5; // Beispiel: Lautstärke reduzieren
          }
        }
      }
  
      return true;
    }
  }
  
  // Registriere den Prozessor
  registerProcessor('custom-audio-processor', CustomAudioProcessor);
  
  // Haupt-Logik in der Anwendung
  (async function setupAudio() {
    if (!window.AudioWorklet) {
      console.error('AudioWorklet wird in diesem Browser nicht unterstützt.');
      return;
    }
  
    const audioContext = new AudioContext({ sampleRate: 44100 });
  
    // Lade den AudioWorklet-Prozessor
    await audioContext.audioWorklet.addModule('audio-processor.js');
  
    // Erstelle eine Instanz des Prozessor-Knotens
    const workletNode = new AudioWorkletNode(audioContext, 'custom-audio-processor', {
      numberOfInputs: 1,
      numberOfOutputs: 1,
      outputChannelCount: [2],
    });
  
    // Stream-Verarbeitung: Audioquelle verbinden
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      const source = audioContext.createMediaStreamSource(stream);
      source.connect(workletNode);
      workletNode.connect(audioContext.destination);
    } catch (error) {
      console.error('Fehler beim Zugriff auf den Audio-Stream:', error);
    }
  
    // Sicherheitskontrolle: CORS-Einschränkungen bei externen Ressourcen
    // Beispiel: XMLHttpRequest, Fetch API nur mit sicheren URLs verwenden
  })();
  