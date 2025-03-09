class DataSyncManager {
    constructor(options = {}) {
      this.serverUrl = options.serverUrl;
      this.queue = []; // Offline queue
      this.retryInterval = options.retryInterval || 5000; // Retry interval in ms
      this.isSyncing = false;
      this.localData = options.localData || {}; // Local storage for offline mode
    }
  
    async syncData(data, resolveConflict) {
      try {
        if (this.isSyncing) {
          throw new Error("Sync is already in progress.");
        }
        this.isSyncing = true;
  
        // Fetch the current server state
        const serverData = await this.fetchServerData();
  
        // Resolve conflicts
        const mergedData = resolveConflict ? resolveConflict(this.localData, serverData) : { ...serverData, ...data };
  
        // Push merged data to the server
        await this.pushDataToServer(mergedData);
  
        // Update local data
        this.localData = mergedData;
  
        console.log("Sync successful.");
      } catch (error) {
        console.error("Sync failed:", error);
        this.enqueueData(data); // Store data in the queue if sync fails
      } finally {
        this.isSyncing = false;
      }
    }
  
    async fetchServerData() {
      try {
        const response = await fetch(`${this.serverUrl}/data`);
        if (!response.ok) {
          throw new Error(`Failed to fetch server data: ${response.statusText}`);
        }
        return await response.json();
      } catch (error) {
        console.error("Error fetching server data:", error);
        throw error;
      }
    }
  
    async pushDataToServer(data) {
      try {
        const response = await fetch(`${this.serverUrl}/data`, {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify(data),
        });
        if (!response.ok) {
          throw new Error(`Failed to push data to server: ${response.statusText}`);
        }
        return await response.json();
      } catch (error) {
        console.error("Error pushing data to server:", error);
        throw error;
      }
    }
  
    enqueueData(data) {
      this.queue.push(data);
      console.log("Data enqueued for later sync:", data);
      this.processQueue();
    }
  
    async processQueue() {
      if (!navigator.onLine) {
        console.log("Offline: Queue processing paused.");
        return;
      }
  
      while (this.queue.length > 0) {
        const data = this.queue.shift();
        try {
          await this.syncData(data);
          console.log("Queued data synced successfully.");
        } catch (error) {
          console.error("Failed to sync queued data. Retrying...");
          this.queue.unshift(data);
          await this.retryAfterInterval();
          break;
        }
      }
    }
  
    async retryAfterInterval() {
      return new Promise((resolve) => setTimeout(resolve, this.retryInterval));
    }
  }
  
  // Example usage
  const dataSyncManager = new DataSyncManager({
    serverUrl: "https://example.com/api",
    retryInterval: 3000,
    localData: { key1: "value1" },
  });
  
  // Example conflict resolution function
  const resolveConflict = (localData, serverData) => {
    // Simple resolution strategy: Local data overrides server data
    return { ...serverData, ...localData };
  };
  
  // Simulate a sync
  (async () => {
    await dataSyncManager.syncData({ key2: "value2" }, resolveConflict);
  })();
  