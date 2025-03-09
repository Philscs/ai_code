// Importiere Crypto-Funktionen für die Verschlüsselung
importScripts('https://cdnjs.cloudflare.com/ajax/libs/crypto-js/4.1.1/crypto-js.min.js');

// Konstanten für Memory Management
const MAX_BATCH_SIZE = 1000000; // Maximum Größe für Batch-Verarbeitung
const ENCRYPTION_KEY = 'your-secure-key-here';

// Hauptfunktion für die Nachrichtenverarbeitung
self.onmessage = async function(e) {
    try {
        const { type, data, options } = e.data;
        
        switch (type) {
            case 'process':
                await processLargeDataset(data, options);
                break;
            case 'encrypt':
                await encryptData(data);
                break;
            default:
                throw new Error('Unbekannter Operationstyp');
        }
    } catch (error) {
        handleError(error);
    }
};

// Verarbeitung großer Datensätze in Batches
async function processLargeDataset(data, options) {
    const batches = splitIntoBatches(data, MAX_BATCH_SIZE);
    const results = [];
    
    for (const batch of batches) {
        // Speicher freigeben nach jeder Batch-Verarbeitung
        await new Promise(resolve => setTimeout(resolve, 0));
        
        const processedBatch = await processBatch(batch, options);
        results.push(processedBatch);
        
        // Fortschritt an Main Thread melden
        self.postMessage({
            type: 'progress',
            progress: (results.length / batches.length) * 100
        });
    }
    
    // Gesamtergebnis zurücksenden
    self.postMessage({
        type: 'complete',
        results: results.flat()
    });
}

// Hilfsfunktion für Batch-Verarbeitung
function splitIntoBatches(data, batchSize) {
    const batches = [];
    for (let i = 0; i < data.length; i += batchSize) {
        batches.push(data.slice(i, i + batchSize));
    }
    return batches;
}

// Einzelne Batch verarbeiten
async function processBatch(batch, options) {
    return batch.map(item => {
        // Beispiel für Datenverarbeitung - an Ihre Bedürfnisse anpassen
        return {
            ...item,
            processed: true,
            timestamp: Date.now()
        };
    });
}

// Verschlüsselung sensitiver Daten
async function encryptData(data) {
    try {
        const encrypted = CryptoJS.AES.encrypt(
            JSON.stringify(data),
            ENCRYPTION_KEY
        ).toString();
        
        self.postMessage({
            type: 'encrypted',
            data: encrypted
        });
    } catch (error) {
        handleError(new Error('Verschlüsselungsfehler: ' + error.message));
    }
}

// Zentrale Fehlerbehandlung
function handleError(error) {
    self.postMessage({
        type: 'error',
        error: {
            message: error.message,
            stack: error.stack
        }
    });
}

// Memory Management - Speicher freigeben
function cleanupMemory() {
    // Explicitly mark for garbage collection when possible
    if (global.gc) {
        global.gc();
    }
}

// Beenden des Workers
self.onunload = function() {
    cleanupMemory();
};