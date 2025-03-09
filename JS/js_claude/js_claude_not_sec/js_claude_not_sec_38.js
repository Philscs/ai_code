class DataSyncManager {
    constructor() {
      this.syncQueue = [];
      this.offlineStorage = new OfflineStorage();
      this.conflictResolver = new ConflictResolver();
      this.retryManager = new RetryManager();
      this.isOnline = navigator.onLine;
      this.version = 0;
  
      // Event Listeners für Online/Offline Status
      window.addEventListener('online', () => this.handleOnlineStatus(true));
      window.addEventListener('offline', () => this.handleOnlineStatus(false));
    }
  
    // Hauptmethode zum Synchronisieren von Daten
    async sync(data, endpoint) {
      try {
        // Versionierung für Delta Updates
        const currentVersion = await this.getCurrentVersion(endpoint);
        
        if (!this.isOnline) {
          await this.handleOfflineSync(data, endpoint);
          return;
        }
  
        // Delta Update: Nur geänderte Daten senden
        const deltaData = this.calculateDelta(data, currentVersion);
        
        if (Object.keys(deltaData).length === 0) {
          console.log('Keine Änderungen zu synchronisieren');
          return;
        }
  
        // Zur Queue hinzufügen
        const syncItem = {
          data: deltaData,
          endpoint,
          timestamp: Date.now(),
          retryCount: 0
        };
        
        this.syncQueue.push(syncItem);
        await this.processSyncQueue();
        
      } catch (error) {
        console.error('Sync Fehler:', error);
        throw error;
      }
    }
  
    // Delta Berechnung
    calculateDelta(newData, version) {
      const delta = {};
      
      for (const [key, value] of Object.entries(newData)) {
        if (this.hasChanged(key, value, version)) {
          delta[key] = value;
        }
      }
      
      return delta;
    }
  
    // Offline Handling
    async handleOfflineSync(data, endpoint) {
      console.log('Offline Modus: Speichere Daten lokal');
      await this.offlineStorage.store({
        data,
        endpoint,
        timestamp: Date.now()
      });
    }
  
    // Online Status Handler
    async handleOnlineStatus(isOnline) {
      this.isOnline = isOnline;
      
      if (isOnline) {
        console.log('Online: Starte Sync von offline Daten');
        const offlineData = await this.offlineStorage.getAll();
        
        for (const item of offlineData) {
          await this.sync(item.data, item.endpoint);
        }
        
        await this.offlineStorage.clear();
      }
    }
  
    // Queue Verarbeitung
    async processSyncQueue() {
      while (this.syncQueue.length > 0) {
        const syncItem = this.syncQueue[0];
        
        try {
          await this.sendToServer(syncItem);
          this.syncQueue.shift(); // Erfolgreich verarbeitetes Item entfernen
          
        } catch (error) {
          const shouldRetry = await this.retryManager.handleError(syncItem, error);
          
          if (!shouldRetry) {
            this.syncQueue.shift(); // Item entfernen wenn keine Wiederholung mehr möglich
            throw error;
          }
        }
      }
    }
  
    // Server Kommunikation
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
        
        // Konfliktprüfung
        if (serverData.conflict) {
          const resolvedData = await this.conflictResolver.resolve(syncItem.data, serverData.serverData);
          return this.sync(resolvedData, syncItem.endpoint);
        }
  
        return serverData;
        
      } catch (error) {
        console.error('Server Kommunikationsfehler:', error);
        throw error;
      }
    }
  
    // Hilfsmethoden
    async getCurrentVersion(endpoint) {
      try {
        const response = await fetch(`${endpoint}/version`);
        const data = await response.json();
        return data.version;
      } catch (error) {
        console.error('Version Abruf fehlgeschlagen:', error);
        return this.version;
      }
    }
  
    hasChanged(key, value, version) {
      // Implementierung der Änderungserkennung
      // Hier könnte z.B. ein Timestamp-Vergleich stattfinden
      return true; // Vereinfachte Implementation
    }
  }
  
  // Offline Storage Klasse
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
  
  // Conflict Resolver Klasse
  class ConflictResolver {
    async resolve(clientData, serverData) {
      // Hier komplexere Konfliktlösungsstrategien implementieren
      const resolved = {};
      
      for (const [key, value] of Object.entries(serverData)) {
        if (clientData[key] !== value) {
          // Beispiel: Server-Version bevorzugen
          resolved[key] = value;
        } else {
          resolved[key] = clientData[key];
        }
      }
      
      return resolved;
    }
  }
  
  // Retry Manager Klasse
  class RetryManager {
    constructor() {
      this.maxRetries = 3;
      this.retryDelay = 1000; // ms
    }
  
    async handleError(syncItem, error) {
      if (syncItem.retryCount >= this.maxRetries) {
        console.log('Maximale Anzahl an Wiederholungsversuchen erreicht');
        return false;
      }
  
      syncItem.retryCount++;
      
      // Exponentielles Backoff
      const delay = this.retryDelay * Math.pow(2, syncItem.retryCount - 1);
      
      await new Promise(resolve => setTimeout(resolve, delay));
      return true;
    }
  }
  
  // Beispiel Verwendung:
  const dataSyncManager = new DataSyncManager();
  
  // Beispieldaten
  const data = {
    id: 1,
    name: 'Test',
    value: 100
  };
  
  // Sync durchführen
  dataSyncManager.sync(data, 'https://api.example.com/sync')
    .then(() => console.log('Sync erfolgreich'))
    .catch(error => console.error('Sync fehlgeschlagen:', error));