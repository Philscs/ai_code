class SecureIndexedDB {
    constructor(dbName, encryptionKey) {
        this.dbName = dbName;
        this.encryptionKey = encryptionKey;
        this.db = null;
    }

    async init(version, migrationCallback) {
        return new Promise((resolve, reject) => {
            const request = indexedDB.open(this.dbName, version);

            request.onupgradeneeded = (event) => {
                this.db = event.target.result;
                if (migrationCallback) {
                    migrationCallback(this.db, event.oldVersion, event.newVersion);
                }
            };

            request.onsuccess = (event) => {
                this.db = event.target.result;
                resolve();
            };

            request.onerror = (event) => {
                reject(event.target.error);
            };
        });
    }

    async encrypt(data) {
        const encoder = new TextEncoder();
        const encodedData = encoder.encode(JSON.stringify(data));
        const encryptedData = await crypto.subtle.encrypt(
            { name: "AES-GCM", iv: encodedData.slice(0, 12) },
            this.encryptionKey,
            encodedData
        );
        return encryptedData;
    }

    async decrypt(data) {
        const decryptedData = await crypto.subtle.decrypt(
            { name: "AES-GCM", iv: data.slice(0, 12) },
            this.encryptionKey,
            data
        );
        const decoder = new TextDecoder();
        return JSON.parse(decoder.decode(decryptedData));
    }

    sanitizeInput(input) {
        if (typeof input !== "string") {
            throw new Error("Invalid input: Only strings are allowed.");
        }
        return input.replace(/[<>"'&]/g, (char) => {
            const charMap = {
                "<": "&lt;",
                ">": "&gt;",
                "\"": "&quot;",
                "'": "&#39;",
                "&": "&amp;",
            };
            return charMap[char] || char;
        });
    }

    async performTransaction(storeName, mode, transactionCallback) {
        return new Promise((resolve, reject) => {
            const transaction = this.db.transaction(storeName, mode);
            const store = transaction.objectStore(storeName);

            transaction.oncomplete = () => resolve();
            transaction.onerror = (event) => reject(event.target.error);

            transactionCallback(store);
        });
    }

    async checkQuota() {
        if (navigator.storage && navigator.storage.estimate) {
            const { usage, quota } = await navigator.storage.estimate();
            return { usage, quota };
        } else {
            throw new Error("Quota management is not supported in this browser.");
        }
    }

    async addData(storeName, data) {
        const sanitizedData = this.sanitizeInput(JSON.stringify(data));
        const encryptedData = await this.encrypt(sanitizedData);
        await this.performTransaction(storeName, "readwrite", (store) => {
            store.add(encryptedData);
        });
    }

    async getData(storeName, key) {
        return new Promise((resolve, reject) => {
            const transaction = this.db.transaction(storeName, "readonly");
            const store = transaction.objectStore(storeName);
            const request = store.get(key);

            request.onsuccess = async (event) => {
                if (event.target.result) {
                    const decryptedData = await this.decrypt(event.target.result);
                    resolve(JSON.parse(decryptedData));
                } else {
                    resolve(null);
                }
            };

            request.onerror = (event) => reject(event.target.error);
        });
    }
}

// Usage Example
(async () => {
    const dbName = "secureDB";
    const encryptionKey = await crypto.subtle.generateKey(
        { name: "AES-GCM", length: 256 },
        true,
        ["encrypt", "decrypt"]
    );

    const db = new SecureIndexedDB(dbName, encryptionKey);

    await db.init(1, (db, oldVersion, newVersion) => {
        if (oldVersion < 1) {
            db.createObjectStore("secureStore", { keyPath: "id", autoIncrement: true });
        }
    });

    await db.addData("secureStore", { message: "Hello, World!" });
    const data = await db.getData("secureStore", 1);
    console.log(data);
})();
