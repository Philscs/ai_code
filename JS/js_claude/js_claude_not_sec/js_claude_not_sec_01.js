class CacheEntry {
    constructor(key, value, maxAge) {
      this.key = key;
      this.value = value;
      this.lastAccessed = Date.now();
      this.createdAt = Date.now();
      this.maxAge = maxAge;
    }
  
    isExpired() {
      return this.maxAge ? Date.now() - this.createdAt > this.maxAge : false;
    }
  }
  
  class APICache {
    constructor(options = {}) {
      // Standardwerte für die Konfiguration
      this.maxSize = options.maxSize || 100; // Maximale Anzahl von Einträgen
      this.maxBytes = options.maxBytes || 5 * 1024 * 1024; // 5MB Standardgröße
      this.defaultMaxAge = options.defaultMaxAge || 5 * 60 * 1000; // 5 Minuten TTL
      
      this.cache = new Map();
      this.currentBytes = 0;
    }
  
    // Hilfsfunktion zur Berechnung der Größe eines Wertes
    getValueSize(value) {
      if (typeof value === 'string') {
        return new Blob([value]).size;
      } else if (typeof value === 'object') {
        return new Blob([JSON.stringify(value)]).size;
      }
      return 0;
    }
  
    // Überprüft, ob genug Speicherplatz vorhanden ist
    hasSpace(newValueSize) {
      return (
        this.cache.size < this.maxSize &&
        this.currentBytes + newValueSize <= this.maxBytes
      );
    }
  
    // Entfernt den am längsten nicht genutzten Eintrag
    evictLRU() {
      let oldestAccessTime = Date.now();
      let oldestKey = null;
  
      for (const [key, entry] of this.cache.entries()) {
        if (entry.lastAccessed < oldestAccessTime) {
          oldestAccessTime = entry.lastAccessed;
          oldestKey = key;
        }
      }
  
      if (oldestKey) {
        const entry = this.cache.get(oldestKey);
        this.currentBytes -= this.getValueSize(entry.value);
        this.cache.delete(oldestKey);
        return true;
      }
      return false;
    }
  
    // Hauptmethode zum Setzen eines Cache-Eintrags
    set(key, value, maxAge = this.defaultMaxAge) {
      // Prüfe auf vorhandenen Eintrag
      if (this.cache.has(key)) {
        const oldEntry = this.cache.get(key);
        this.currentBytes -= this.getValueSize(oldEntry.value);
      }
  
      const valueSize = this.getValueSize(value);
  
      // Wenn der neue Wert zu groß ist
      if (valueSize > this.maxBytes) {
        console.warn(`Wert für Key "${key}" ist zu groß für den Cache`);
        return false;
      }
  
      // Räume Platz frei, wenn nötig
      while (!this.hasSpace(valueSize)) {
        if (!this.evictLRU()) {
          console.warn('Cache konnte nicht genügend Platz freimachen');
          return false;
        }
      }
  
      // Erstelle neuen Cache-Eintrag
      const entry = new CacheEntry(key, value, maxAge);
      this.cache.set(key, entry);
      this.currentBytes += valueSize;
  
      return true;
    }
  
    // Abrufen eines Cache-Eintrags
    get(key) {
      const entry = this.cache.get(key);
      
      if (!entry) {
        return null;
      }
  
      // Prüfe auf Ablauf
      if (entry.isExpired()) {
        this.delete(key);
        return null;
      }
  
      // Aktualisiere Zugriffszeitpunkt
      entry.lastAccessed = Date.now();
      return entry.value;
    }
  
    // Löschen eines Cache-Eintrags
    delete(key) {
      const entry = this.cache.get(key);
      if (entry) {
        this.currentBytes -= this.getValueSize(entry.value);
        this.cache.delete(key);
        return true;
      }
      return false;
    }
  
    // Cache leeren
    clear() {
      this.cache.clear();
      this.currentBytes = 0;
    }
  
    // Cache-Statistiken
    getStats() {
      return {
        size: this.cache.size,
        bytes: this.currentBytes,
        maxSize: this.maxSize,
        maxBytes: this.maxBytes,
        utilizationPercentage: (this.currentBytes / this.maxBytes) * 100
      };
    }
  }
  
  // Beispielnutzung:
  const cache = new APICache({
    maxSize: 1000,
    maxBytes: 10 * 1024 * 1024, // 10MB
    defaultMaxAge: 10 * 60 * 1000 // 10 Minuten
  });
  
  // Beispiel für die Integration mit fetch
  async function cachedFetch(url, options = {}) {
    const cacheKey = `${options.method || 'GET'}-${url}`;
    
    // Prüfe Cache
    const cachedResponse = cache.get(cacheKey);
    if (cachedResponse) {
      return cachedResponse;
    }
  
    // Führe Request aus
    try {
      const response = await fetch(url, options);
      const data = await response.json();
      
      // Cache das Ergebnis
      cache.set(cacheKey, data);
      
      return data;
    } catch (error) {
      console.error('Fetch error:', error);
      throw error;
    }
  }