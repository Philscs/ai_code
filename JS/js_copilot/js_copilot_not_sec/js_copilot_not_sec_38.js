class DataSyncManager {
    constructor() {
        this.syncQueue = [];
        this.offlineStorage = new OfflineStorage();
        this.conflictResolver = new ConflictResolver();
        this.retryManager = new RetryManager();
        this.isOnline = navigator.onLine;
        this.version = 0;

        window.addEventListener('online', () => this.handleOnlineStatus(true));
        window.addEventListener('offline', () => this.handleOnlineStatus(false));
    }

    async sync(data, endpoint) {
        try {
            const currentVersion = await this.getCurrentVersion(endpoint);

            if (!this.isOnline) {
                await this.handleOfflineSync(data, endpoint);
                return;
            }

            const deltaData = this.calculateDelta(data, currentVersion);

            if (Object.keys(deltaData).length === 0) {
                console.log('No changes to synchronize');
                return;
            }

            const syncItem = {
                data: deltaData,
                endpoint,
                timestamp: Date.now(),
                retryCount: 0
            };

            this.syncQueue.push(syncItem);
            await this.processSyncQueue();

        } catch (error) {
            console.error('Sync error:', error);
            throw error;
        }
    }

    calculateDelta(newData, version) {
        const delta = {};

        for (const [key, value] of Object.entries(newData)) {
            if (this.hasChanged(key, value, version)) {
                delta[key] = value;
            }
        }

        return delta;
    }

    async handleOfflineSync(data, endpoint) {
        console.log('Offline mode: Storing data locally');
        await this.offlineStorage.store({
            data,
            endpoint,
            timestamp: Date.now()
        });
    }

    async handleOnlineStatus(isOnline) {
        this.isOnline = isOnline;

        if (isOnline) {
            console.log('Online: Starting sync of offline data');
            const offlineData = await this.offlineStorage.getAll();

            for (const item of offlineData) {
                await this.sync(item.data, item.endpoint);
            }

            await this.offlineStorage.clear();
        }
    }

    async processSyncQueue() {
        while (this.syncQueue.length > 0) {
            const syncItem = this.syncQueue[0];

            try {
                await this.sendToServer(syncItem);
                this.syncQueue.shift();

            } catch (error) {
                const shouldRetry = await this.retryManager.handleError(syncItem, error);

                if (!shouldRetry) {
                    this.syncQueue.shift();
                    throw error;
                }
            }
        }
    }

    async sendToServer(syncItem) {
        try {
            const response = await fetch(syncItem.endpoint, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-Client-Version': this.version.toString()
                },
                body: JSON.stringify(syncItem.data)
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            const serverData = await response.json();

            if (serverData.conflict) {
                const resolvedData = await this.conflictResolver.resolve(syncItem.data, serverData.serverData);
                return this.sync(resolvedData, syncItem.endpoint);
            }

            return serverData;

        } catch (error) {
            console.error('Server communication error:', error);
            throw error;
        }
    }

    async getCurrentVersion(endpoint) {
        try {
            const response = await fetch(`${endpoint}/version`);
            const data = await response.json();
            return data.version;
        } catch (error) {
            console.error('Failed to retrieve version:', error);
            return this.version;
        }
    }

    hasChanged(key, value, version) {
        return true; // Simplified implementation
    }
}

class OfflineStorage {
    constructor() {
        this.storageKey = 'offlineSync';
    }

    async store(data) {
        const storedData = await this.getAll();
        storedData.push(data);
        localStorage.setItem(this.storageKey, JSON.stringify(storedData));
    }

    async getAll() {
        const data = localStorage.getItem(this.storageKey);
        return data ? JSON.parse(data) : [];
    }

    async clear() {
        localStorage.removeItem(this.storageKey);
    }
}

class ConflictResolver {
    async resolve(clientData, serverData) {
        const resolved = {};

        for (const [key, value] of Object.entries(serverData)) {
            resolved[key] = clientData[key] !== value ? value : clientData[key];
        }

        return resolved;
    }
}

class RetryManager {
    constructor() {
        this.maxRetries = 3;
        this.retryDelay = 1000; // ms
    }

    async handleError(syncItem, error) {
        if (syncItem.retryCount >= this.maxRetries) {
            console.log('Maximum number of retries reached');
            return false;
        }

        syncItem.retryCount++;

        const delay = this.retryDelay * Math.pow(2, syncItem.retryCount - 1);

        await new Promise(resolve => setTimeout(resolve, delay));
        return true;
    }
}

const dataSyncManager = new DataSyncManager();

const data = {
    id: 1,
    name: 'Test',
    value: 100
};

dataSyncManager.sync(data, 'https://api.example.com/sync')
    .then(() => console.log('Sync successful'))
    .catch(error => console.error('Sync failed:', error));
