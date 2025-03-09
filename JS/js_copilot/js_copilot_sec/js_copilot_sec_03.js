self.onmessage = async function(e) {
    const data = e.data;
    
    try {
        const result = await processData(data);
        self.postMessage(result);
    } catch (error) {
        handleError(error);
    }

    try {
        const encryptedData = await encryptData(data);
        self.postMessage(encryptedData);
    } catch (error) {
        handleError(error);
    }

    self.close();
};

async function encryptData(data) {
    const encoder = new TextEncoder();
    const password = "your_password"; // Replace with your own password
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

function processData(data) {
    return data.map(item => {
        return { ...item, processed: true };
    });
}

function handleError(error) {
    console.error("Fehler im Worker:", error);
    self.postMessage({ error: error.message });
}
