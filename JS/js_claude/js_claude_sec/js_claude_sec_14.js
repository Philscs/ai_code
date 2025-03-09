// Secure IndexedDB Wrapper Library

// Encryption utilities using Web Crypto API
class CryptoLayer {
    constructor() {
        this.algorithm = {
            name: 'AES-GCM',
            length: 256
        };
    }

    async generateKey() {
        return await window.crypto.subtle.generateKey(
            this.algorithm,
            true,
            ['encrypt', 'decrypt']
        );
    }

    async encrypt(data, key) {
        const iv = window.crypto.getRandomValues(new Uint8Array(12));
        const encodedData = new TextEncoder().encode(JSON.stringify(data));

        const encryptedData = await window.crypto.subtle.encrypt(
            {
                name: this.algorithm.name,
                iv
            },
            key,
            encodedData
        );

        return {
            data: Array.from(new Uint8Array(encryptedData)),
            iv: Array.from(iv)
        };
    }

    async decrypt(encryptedData, key, iv) {
        const decryptedData = await window.crypto.subtle.decrypt(
            {
                name: this.algorithm.name,
                iv: new Uint8Array(iv)
            },
            key,
            new Uint8Array(encryptedData)
        );

        return JSON.parse(new TextDecoder().decode(decryptedData));
    }
}

// Data sanitization utilities
class Sanitizer {
    static sanitizeInput(data) {
        if (typeof data === 'string') {
            return this.sanitizeString(data);
        } else if (typeof data === 'object' && data !== null) {
            return this.sanitizeObject(data);
        }
        return data;
    }

    static sanitizeString(str) {
        return str.replace(/<[^>]*>/g, '') // Remove HTML tags
                 .replace(/[^\w\s-]/g, '') // Remove special characters
                 .trim();
    }

    static sanitizeObject(obj) {
        const sanitized = {};
        for (const [key, value] of Object.entries(obj)) {
            if (typeof value === 'string') {
                sanitized[key] = this.sanitizeString(value);
            } else if (typeof value === 'object' && value !== null) {
                sanitized[key] = this.sanitizeObject(value);
            } else {
                sanitized[key] = value;
            }
        }
        return sanitized;
    }
}

// Migration system
class MigrationManager {
    constructor(db) {
        this.db = db;
        this.migrations = new Map();
    }

    addMigration(version, callback) {
        this.migrations.set(version, callback);
    }

    async migrate(oldVersion, newVersion) {
        for (let v = oldVersion + 1; v <= newVersion; v++) {
            const migration = this.migrations.get(v);
            if (migration) {
                await migration(this.db);
            }
        }
    }
}

// Quota management
class QuotaManager {
    constructor(maxSize = 50 * 1024 * 1024) { // Default 50MB
        this.maxSize = maxSize;
    }

    async checkQuota() {
        if ('storage' in navigator && 'estimate' in navigator.storage) {
            const estimate = await navigator.storage.estimate();
            return {
                usage: estimate.usage,
                quota: estimate.quota,
                available: estimate.quota - estimate.usage
            };
        }
        throw new Error('Storage estimation not supported');
    }

    async hasAvailableSpace(requiredSpace) {
        const { available } = await this.checkQuota();
        return available >= requiredSpace;
    }
}

// Main wrapper class
class SecureIndexedDB {
    constructor(dbName, version) {
        this.dbName = dbName;
        this.version = version;
        this.db = null;
        this.crypto = new CryptoLayer();
        this.quotaManager = new QuotaManager();
        this.migrationManager = null;
        this.encryptionKey = null;
    }

    async init() {
        if (!window.indexedDB) {
            throw new Error('IndexedDB not supported');
        }

        return new Promise((resolve, reject) => {
            const request = indexedDB.open(this.dbName, this.version);

            request.onerror = () => reject(request.error);
            request.onsuccess = () => {
                this.db = request.result;
                resolve();
            };

            request.onupgradeneeded = (event) => {
                this.db = event.target.result;
                this.migrationManager = new MigrationManager(this.db);
                this.migrationManager.migrate(event.oldVersion, this.version);
            };
        });
    }

    async setEncryptionKey(key) {
        this.encryptionKey = key || await this.crypto.generateKey();
    }

    async put(storeName, data, key = null) {
        const sanitizedData = Sanitizer.sanitizeInput(data);
        const encryptedData = await this.crypto.encrypt(sanitizedData, this.encryptionKey);

        const size = JSON.stringify(encryptedData).length;
        if (!(await this.quotaManager.hasAvailableSpace(size))) {
            throw new Error('Storage quota exceeded');
        }

        return new Promise((resolve, reject) => {
            const transaction = this.db.transaction(storeName, 'readwrite');
            const store = transaction.objectStore(storeName);

            const request = key ? store.put(encryptedData, key) : store.put(encryptedData);

            request.onerror = () => reject(request.error);
            request.onsuccess = () => resolve(request.result);

            transaction.onerror = () => reject(transaction.error);
            transaction.onabort = () => reject(new Error('Transaction aborted'));
        });
    }

    async get(storeName, key) {
        return new Promise((resolve, reject) => {
            const transaction = this.db.transaction(storeName, 'readonly');
            const store = transaction.objectStore(storeName);
            const request = store.get(key);

            request.onerror = () => reject(request.error);
            request.onsuccess = async () => {
                if (!request.result) {
                    resolve(null);
                    return;
                }

                try {
                    const decryptedData = await this.crypto.decrypt(
                        request.result.data,
                        this.encryptionKey,
                        request.result.iv
                    );
                    resolve(decryptedData);
                } catch (error) {
                    reject(error);
                }
            };
        });
    }

    async delete(storeName, key) {
        return new Promise((resolve, reject) => {
            const transaction = this.db.transaction(storeName, 'readwrite');
            const store = transaction.objectStore(storeName);
            const request = store.delete(key);

            request.onerror = () => reject(request.error);
            request.onsuccess = () => resolve();
        });
    }

    async clear(storeName) {
        return new Promise((resolve, reject) => {
            const transaction = this.db.transaction(storeName, 'readwrite');
            const store = transaction.objectStore(storeName);
            const request = store.clear();

            request.onerror = () => reject(request.error);
            request.onsuccess = () => resolve();
        });
    }

    async transaction(storeNames, mode, callback) {
        return new Promise((resolve, reject) => {
            const transaction = this.db.transaction(storeNames, mode);
            
            transaction.onerror = () => reject(transaction.error);
            transaction.onabort = () => reject(new Error('Transaction aborted'));
            
            try {
                const result = callback(transaction);
                transaction.oncomplete = () => resolve(result);
            } catch (error) {
                reject(error);
            }
        });
    }
}

// Usage example
async function example() {
    const db = new SecureIndexedDB('myDB', 1);
    await db.init();
    await db.setEncryptionKey();

    // Define a migration
    db.migrationManager.addMigration(1, (database) => {
        database.createObjectStore('users', { keyPath: 'id' });
    });

    // Store encrypted data
    await db.put('users', {
        id: 1,
        username: 'john_doe',
        email: 'john@example.com'
    });

    // Retrieve and decrypt data
    const userData = await db.get('users', 1);

    // Using transaction
    await db.transaction(['users'], 'readwrite', (tx) => {
        const store = tx.objectStore('users');
        store.put({ id: 2, username: 'jane_doe' });
        store.put({ id: 3, username: 'bob_smith' });
    });
}