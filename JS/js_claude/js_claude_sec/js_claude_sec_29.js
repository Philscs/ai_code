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
                ...this.algorithm,
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
        const decrypted = await window.crypto.subtle.decrypt(
            {
                ...this.algorithm,
                iv: new Uint8Array(iv)
            },
            key,
            new Uint8Array(encryptedData)
        );

        return JSON.parse(new TextDecoder().decode(decrypted));
    }
}

// Version control management
class VersionControl {
    constructor() {
        this.versions = new Map();
    }

    createVersion(key, data) {
        const version = {
            timestamp: Date.now(),
            data: data,
            version: this.getCurrentVersion(key) + 1
        };
        
        if (!this.versions.has(key)) {
            this.versions.set(key, []);
        }
        
        this.versions.get(key).push(version);
        return version.version;
    }

    getCurrentVersion(key) {
        const versions = this.versions.get(key);
        return versions ? versions.length : 0;
    }

    getVersion(key, version) {
        const versions = this.versions.get(key);
        return versions ? versions.find(v => v.version === version) : null;
    }

    getAllVersions(key) {
        return this.versions.get(key) || [];
    }
}

// Access control management
class AccessControl {
    constructor() {
        this.permissions = new Map();
    }

    setPermissions(key, rules) {
        this.permissions.set(key, rules);
    }

    checkPermission(key, operation, user) {
        const rules = this.permissions.get(key);
        if (!rules) return false;

        const userRules = rules[user.role] || [];
        return userRules.includes(operation);
    }
}

// Storage quota management
class QuotaManager {
    constructor(maxQuota = 10 * 1024 * 1024) { // Default 10MB
        this.maxQuota = maxQuota;
        this.currentUsage = 0;
    }

    async checkQuota(size) {
        const available = await this.getAvailableSpace();
        return size <= available;
    }

    async getAvailableSpace() {
        if (navigator.storage && navigator.storage.estimate) {
            const estimate = await navigator.storage.estimate();
            this.currentUsage = estimate.usage || 0;
            return Math.max(0, this.maxQuota - this.currentUsage);
        }
        return this.maxQuota - this.currentUsage;
    }

    updateUsage(size) {
        this.currentUsage += size;
    }
}

// Garbage collection
class GarbageCollector {
    constructor(storage) {
        this.storage = storage;
        this.maxVersions = 5;
        this.maxAge = 30 * 24 * 60 * 60 * 1000; // 30 days
    }

    async collect() {
        const keys = await this.storage.getAllKeys();
        
        for (const key of keys) {
            const versions = await this.storage.versionControl.getAllVersions(key);
            
            // Remove old versions
            const sortedVersions = versions.sort((a, b) => b.timestamp - a.timestamp);
            
            if (sortedVersions.length > this.maxVersions) {
                const versionsToRemove = sortedVersions.slice(this.maxVersions);
                for (const version of versionsToRemove) {
                    await this.storage.deleteVersion(key, version.version);
                }
            }

            // Remove expired data
            const now = Date.now();
            for (const version of versions) {
                if (now - version.timestamp > this.maxAge) {
                    await this.storage.deleteVersion(key, version.version);
                }
            }
        }
    }
}

// Main storage management system
class StorageManager {
    constructor(options = {}) {
        this.crypto = new CryptoLayer();
        this.versionControl = new VersionControl();
        this.accessControl = new AccessControl();
        this.quotaManager = new QuotaManager(options.maxQuota);
        this.garbageCollector = new GarbageCollector(this);
        this.key = null;
    }

    async initialize() {
        this.key = await this.crypto.generateKey();
        
        // Start periodic garbage collection
        setInterval(() => {
            this.garbageCollector.collect();
        }, 24 * 60 * 60 * 1000); // Daily cleanup
    }

    async set(key, value, user) {
        if (!this.accessControl.checkPermission(key, 'write', user)) {
            throw new Error('Access denied');
        }

        const serializedData = JSON.stringify(value);
        const dataSize = new Blob([serializedData]).size;

        if (!await this.quotaManager.checkQuota(dataSize)) {
            throw new Error('Storage quota exceeded');
        }

        const encryptedData = await this.crypto.encrypt(value, this.key);
        const version = this.versionControl.createVersion(key, encryptedData);

        try {
            localStorage.setItem(
                `${key}_v${version}`,
                JSON.stringify(encryptedData)
            );
            this.quotaManager.updateUsage(dataSize);
            return version;
        } catch (error) {
            throw new Error('Storage operation failed');
        }
    }

    async get(key, version, user) {
        if (!this.accessControl.checkPermission(key, 'read', user)) {
            throw new Error('Access denied');
        }

        const targetVersion = version || this.versionControl.getCurrentVersion(key);
        const storedData = localStorage.getItem(`${key}_v${targetVersion}`);

        if (!storedData) {
            return null;
        }

        const { data, iv } = JSON.parse(storedData);
        return await this.crypto.decrypt(data, this.key, iv);
    }

    async deleteVersion(key, version) {
        localStorage.removeItem(`${key}_v${version}`);
    }

    async getAllKeys() {
        return Object.keys(localStorage)
            .filter(key => key.includes('_v'))
            .map(key => key.split('_v')[0]);
    }
}

// Usage example
async function example() {
    const storage = new StorageManager({ maxQuota: 5 * 1024 * 1024 }); // 5MB quota
    await storage.initialize();

    // Set up access control
    storage.accessControl.setPermissions('userPreferences', {
        admin: ['read', 'write'],
        user: ['read']
    });

    const user = { role: 'admin' };

    try {
        // Store encrypted data with version control
        const version = await storage.set('userPreferences', {
            theme: 'dark',
            language: 'de'
        }, user);

        // Retrieve latest version
        const data = await storage.get('userPreferences', null, user);
        console.log('Retrieved data:', data);

        // Retrieve specific version
        const oldVersion = await storage.get('userPreferences', version, user);
        console.log('Old version:', oldVersion);
    } catch (error) {
        console.error('Storage operation failed:', error);
    }
}