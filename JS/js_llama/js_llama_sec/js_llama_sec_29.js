class QuotaHandler {
    constructor(storage) {
      this.storage = storage;
      this.quota = 100 * 1024; // 100 KB
      this.currentUsage = 0;
    }
  
    getQuota() {
      return this.quota;
    }
  
    getUsedSpace() {
      return this.currentUsage;
    }
  
    setQuota(newQuota) {
      this.quota = newQuota;
      this.currentUsage = Math.min(this.currentUsage, this.quota);
    }
  
    addData(data) {
      const size = JSON.stringify(data).length;
      if (size > this.quota - this.currentUsage) {
        throw new Error("Quota erreicht!");
      }
      this.storage.setItem("_quota_used", size);
      this.currentUsage += size;
    }
  
    removeData() {
      const size = parseInt(this.storage.getItem("_quota_used"), 10);
      if (size === 0) {
        return;
      }
      this.storage.removeItem("_quota_used");
      this.currentUsage -= size;
    }
  }
  class EncryptionLayer {
    constructor(key) {
      this.key = key;
      this.iv = new Uint8Array(16);
    }
  
    encrypt(data) {
      const encryptedData = new TextEncoder().encode(data);
      const iv = crypto.getRandomValues(this.iv);
      return new Uint8Array([...iv, ...encryptedData]);
    }
  
    decrypt(encryptedData) {
      const iv = encryptedData.slice(0, 16);
      const data = new Uint8Array(encryptedData.slice(16));
      return new TextDecoder().decode(new Uint8Array([...data, ...iv]));
    }
  }
  class GarbageCollector {
    constructor(storage) {
      this.storage = storage;
      this.garbageList = [];
    }
  
    mark() {
      // Mark alle verwendetem Speicher
      const usedStorage = {};
      for (const key in this.storage) {
        if (this.storage[key] !== null && typeof this.storage[key] === "object") {
          Object.keys(this.storage[key]).forEach((key2) => {
            usedStorage[key2] = true;
          });
        }
      }
      // Mark alle nicht verwendeten Speicher
      for (const key in this.garbageList) {
        if (!usedStorage[key]) {
          delete this.storage[key];
        } else {
          this.garbageList.splice(key, 1);
        }
      }
    }
  
    collect() {
      this.mark();
      // Ältere Daten entfernen
      const retentionTime = 30 * 24 * 60 * 60 * 1000; // 30 Tage
      for (const key in this.storage) {
        if (this.storage[key] !== null && typeof this.storage[key] === "object") {
          const timestamp = new Date(this.storage[key].timestamp).getTime();
          if (timestamp < retentionTime) {
            delete this.storage[key];
          }
        }
      }
    }
  }
  class AccessControl {
    constructor(storage) {
      this.storage = storage;
      this.roles = {};
    }
  
    addRole(roleName, permissions) {
      this.roles[roleName] = permissions;
    }
  
    checkPermission(roleName, action) {
      if (this.roles[roleName]) {
        const permission = this.roles[roleName][action];
        if (permission === "allow") {
          return true;
        } else if (permission === "deny") {
          return false;
        }
      }
      return true; // Default-allowed
    }
  
    restrictAccess() {
      for (const key in this.storage) {
        const accessLevel = this.checkPermission(key, "access");
        if (!accessLevel) {
          delete this.storage[key];
        }
      }
    }
  }
  class VersionControl {
    constructor(storage) {
      this.storage = storage;
    }
  
    recordChange(key, version) {
      const currentValue = this.storage[key];
      if (currentValue === null || typeof currentValue !== "object") {
        return false;
      }
      this.storage[key].version = version;
      this.storage[key].changes.push({ type: "add", data: currentValue });
      return true;
    }
  
    restore(key, version) {
      const currentValue = this.storage[key];
      if (currentValue === null || typeof currentValue !== "object") {
        return false;
      }
      this.storage[key] = currentValue.changes[version - 1].data;
      return true;
    }
  
    getVersions(key) {
      const versions = {};
      for (const version in this.storage[key]) {
        versions[version] = JSON.parse(this.storage[key][version]);
      }
      return versions;
    }
  }
  class StorageManager {
    constructor() {
      this.storage = {};
      this.quotaHandler = new QuotaHandler(this.storage);
      this.encryptionLayer = new EncryptionLayer("my_secret_key");
      this.garbageCollector = new GarbageCollector(this.storage);
      this.accessControl = new AccessControl(this.storage);
      this.versionControl = new VersionControl(this.storage);
    }
  
    addData(data) {
      const encryptedData = this.encryptionLayer.encrypt(JSON.stringify(data));
      if (this.quotaHandler.getUsedSpace() + encryptedData.length > this.quotaHandler.getQuota()) {
        throw new Error("Quota erreicht!");
      }
      this.storage["data"] = encryptedData;
    }
  
    removeData(key) {
      const data = JSON.parse(this.storage[key]);
      if (this.garbageCollector.checkForGarbage(data)) {
        delete this.storage[key];
      } else {
        for (const role in this.accessControl.roles) {
          if (!this.accessControl.checkPermission(role, "remove")) {
            return;
          }
        }
        delete this.storage[key];
      }
    }
  
    recordChange(key, version) {
      const data = JSON.parse(this.storage[key]);
      return this.versionControl.recordChange(key, version);
    }
  
    restore(key, version) {
      const data = JSON.parse(this.versionControl.restore(key, version));
      return this.versionControl.getVersions(key)[version];
    }
  }
  
  const storageManager = new StorageManager();
  storageManager.addData({ key: "value" });