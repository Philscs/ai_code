// Import notwendige Module
const fs = require('fs');
const path = require('path');
const EventEmitter = require('events');

class FileSystem extends EventEmitter {
    constructor() {
        super();
        this.backends = {};
    }

    // Backend registrieren
    registerBackend(name, backend) {
        this.backends[name] = backend;
    }

    // Backend auswählen
    useBackend(name) {
        if (!this.backends[name]) {
            throw new Error(`Backend ${name} ist nicht registriert.`);
        }
        this.currentBackend = this.backends[name];
    }

    // Pfadauflösung
    resolvePath(filePath) {
        return path.resolve(filePath);
    }

    // Datei lesen (Streaming)
    createReadStream(filePath, options = {}) {
        const resolvedPath = this.resolvePath(filePath);
        return this.currentBackend.createReadStream(resolvedPath, options);
    }

    // Datei schreiben (Streaming)
    createWriteStream(filePath, options = {}) {
        const resolvedPath = this.resolvePath(filePath);
        return this.currentBackend.createWriteStream(resolvedPath, options);
    }

    // Dateiüberwachung
    watch(filePath, options = {}) {
        const resolvedPath = this.resolvePath(filePath);
        return this.currentBackend.watch(resolvedPath, options);
    }
}

// Beispiel-Backend: Lokales Dateisystem
class LocalBackend {
    createReadStream(filePath, options) {
        return fs.createReadStream(filePath, options);
    }

    createWriteStream(filePath, options) {
        return fs.createWriteStream(filePath, options);
    }

    watch(filePath, options) {
        return fs.watch(filePath, options);
    }
}

// Beispiel-Backend: Virtuelles Dateisystem
class VirtualBackend {
    constructor() {
        this.files = new Map();
    }

    createReadStream(filePath) {
        if (!this.files.has(filePath)) {
            throw new Error(`Datei ${filePath} existiert nicht.`);
        }
        const stream = new Readable();
        stream.push(this.files.get(filePath));
        stream.push(null); // Signalisiert das Ende des Streams
        return stream;
    }

    createWriteStream(filePath) {
        const chunks = [];
        const stream = new Writable({
            write(chunk, encoding, callback) {
                chunks.push(chunk);
                callback();
            },
            final(callback) {
                this.files.set(filePath, Buffer.concat(chunks).toString());
                callback();
            },
        });
        return stream;
    }

    watch(filePath, options) {
        const emitter = new EventEmitter();
        // Virtuelle Dateiüberwachung implementieren
        // Diese Implementierung ist ein Platzhalter.
        return emitter;
    }
}

// Nutzung des File-System-Layers
const fileSystem = new FileSystem();

// Lokales Backend registrieren
const localBackend = new LocalBackend();
fileSystem.registerBackend('local', localBackend);

// Virtuelles Backend registrieren
const virtualBackend = new VirtualBackend();
fileSystem.registerBackend('virtual', virtualBackend);

// Backend wechseln und Operationen ausführen
fileSystem.useBackend('local');

const readStream = fileSystem.createReadStream('./example.txt');
readStream.on('data', (chunk) => {
    console.log(`Gelesen: ${chunk}`);
});
