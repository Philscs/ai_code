// Web Worker zur parallelen Verarbeitung und Verschlüsselung

// Funktion zur Verschlüsselung (AES-Beispiel mit Crypto API)
async function encryptData(data, password) {
    const encoder = new TextEncoder();
    const keyMaterial = await crypto.subtle.importKey(
        "raw",
        encoder.encode(password),
        { name: "PBKDF2" },
        false,
        ["deriveKey"]
    );

    const salt = crypto.getRandomValues(new Uint8Array(16));
    const key = await crypto.subtle.deriveKey(
        {
            name: "PBKDF2",
            salt,
            iterations: 100000,
            hash: "SHA-256"
        },
        keyMaterial,
        { name: "AES-GCM", length: 256 },
        false,
        ["encrypt"]
    );

    const iv = crypto.getRandomValues(new Uint8Array(12));
    const encrypted = await crypto.subtle.encrypt(
        {
            name: "AES-GCM",
            iv
        },
        key,
        encoder.encode(JSON.stringify(data))
    );

    return { encryptedData: new Uint8Array(encrypted), iv, salt };
}

// Funktion zur Verarbeitung großer Datensätze
function processLargeDataset(dataset) {
    return dataset.map(item => {
        // Beispiel: Transformation von Daten
        return { ...item, processed: true };
    });
}

// Fehlerbehandlung und Speicherbereinigung
function handleError(error) {
    console.error("Fehler im Worker:", error);
    self.postMessage({ error: error.message });
}

self.onmessage = async function (e) {
    try {
        const { dataset, operation, password } = e.data;

        if (!dataset || !Array.isArray(dataset)) {
            throw new Error("Ungültige oder fehlende Datensätze.");
        }

        if (operation === "process") {
            const processedData = processLargeDataset(dataset);
            self.postMessage({ status: "success", result: processedData });
        } else if (operation === "encrypt") {
            if (!password) {
                throw new Error("Passwort für Verschlüsselung fehlt.");
            }
            const encrypted = await encryptData(dataset, password);
            self.postMessage({ status: "success", result: encrypted });
        } else {
            throw new Error("Unbekannte Operation.");
        }

    } catch (error) {
        handleError(error);
    } finally {
        // Speicherbereinigung falls notwendig
        self.close(); // Optional: Worker beenden
    }
};
