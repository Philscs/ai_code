class BrowserStorageManager {
    constructor(storageType = 'localStorage', encryptionKey = 'defaultKey') {
        this.storage = storageType === 'sessionStorage' ? sessionStorage : localStorage;
        this.encryptionKey = encryptionKey;
        this.version = 1;
        this.accessControl = {}; // Format: { keyName: ['read', 'write'] }
    }

    // Quota Handling
    hasQuota(key, data) {
        const quota = 5 * 1024 * 1024; // 5 MB
        const size = new Blob([JSON.stringify({ key, data })]).size;
        return size + this.getUsedStorageSize() <= quota;
    }

    getUsedStorageSize() {
        return Object.keys(this.storage).reduce((acc, key) => {
            return acc + this.storage.getItem(key).length;
        }, 0);
    }

    // Encryption Layer
    encrypt(data) {
        const encryptedData = btoa(unescape(encodeURIComponent(data)));
        return encryptedData;
    }

    decrypt(data) {
        const decryptedData = decodeURIComponent(escape(atob(data)));
        return decryptedData;
    }

    // Garbage Collection (removes unused/expired data)
    garbageCollect() {
        const now = Date.now();
        Object.keys(this.storage).forEach((key) => {
            const item = JSON.parse(this.decrypt(this.storage.getItem(key)));
            if (item.expiry && item.expiry < now) {
                this.storage.removeItem(key);
            }
        });
    }

    // Access Control
    setAccessControl(key, permissions) {
        this.accessControl[key] = permissions;
    }

    hasPermission(key, action) {
        return this.accessControl[key]?.includes(action) || false;
    }

    // Version Control
    setVersion(version) {
        this.version = version;
    }

    save(key, value, options = { expiry: null }) {
        if (!this.hasPermission(key, 'write')) {
            throw new Error('Write access denied.');
        }

        if (!this.hasQuota(key, value)) {
            throw new Error('Storage quota exceeded.');
        }

        const data = {
            version: this.version,
            value,
            expiry: options.expiry ? Date.now() + options.expiry : null
        };

        const encryptedData = this.encrypt(JSON.stringify(data));
        this.storage.setItem(key, encryptedData);
    }

    load(key) {
        if (!this.hasPermission(key, 'read')) {
            throw new Error('Read access denied.');
        }

        const encryptedData = this.storage.getItem(key);
        if (!encryptedData) return null;

        const data = JSON.parse(this.decrypt(encryptedData));

        if (data.expiry && Date.now() > data.expiry) {
            this.storage.removeItem(key);
            return null;
        }

        return data.version === this.version ? data.value : null;
    }

    remove(key) {
        if (!this.hasPermission(key, 'write')) {
            throw new Error('Write access denied.');
        }

        this.storage.removeItem(key);
    }

    clearAll() {
        this.storage.clear();
    }
}

// Example usage
const manager = new BrowserStorageManager('localStorage', 'myEncryptionKey');

// Set access control
manager.setAccessControl('userSettings', ['read', 'write']);

// Save data
manager.save('userSettings', { theme: 'dark', language: 'de' }, { expiry: 1000 * 60 * 60 }); // 1 hour

// Load data
console.log(manager.load('userSettings'));

// Perform garbage collection
manager.garbageCollect();

// Update version
manager.setVersion(2);
