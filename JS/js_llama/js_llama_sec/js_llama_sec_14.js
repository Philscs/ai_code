class IndexDBWrapper {
    constructor(dbName, version = '1') {
      this.dbName = dbName;
      this.version = version;
      this.transaktionen = [];
      this.quota = null;
      this.data = {};
    }
  
    // Funktion zum Öffnen des IndexDB-Verwalters
    open() {
      const db = window.indexedDB.open(this.dbName, this.version);
      return new Promise((resolve, reject) => {
        db.onsuccess = () => {
          resolve(db.result);
        };
        db.onerror = (error) => {
          reject(error);
        };
      });
    }
  
    // Funktion zum Verwalten der Transaktionen
    transaction(transactionName) {
      const transaktion = new transaction();
      this.transaktionen.push(transaktion);
      return transaktion;
    }
  
    // Funktion zum Managen der Quoten
    quota(quotaSize) {
      this.quota = quotaSize;
      return this;
    }
  
    // Funktion zum Sanitieren von Daten
    sanitize(data) {
      if (data === null || data === undefined) {
        return '';
      }
      return JSON.stringify(data);
    }
  
    // Funktion zum Speichern der Daten im IndexDB-Verwalter
    save() {
      const db = window.indexedDB;
      const transaction = this.transaktionen[0];
      if (transaction) {
        transaction.start();
        const store = db.transaction(transactionName, 'readonly').objectStore('data');
        for (const key in this.data) {
          store.add(this.sanitize(this.data[key]));
        }
        transaction.commit();
        return true;
      } else {
        return false;
      }
    }
  
    // Funktion zum Abrufen der Daten aus dem IndexDB-Verwalter
    load() {
      const db = window.indexedDB;
      const transaction = this.transaktionen[0];
      if (transaction) {
        transaction.start();
        const store = db.transaction(transactionName, 'readonly').objectStore('data');
        const request = store.getAll();
        return new Promise((resolve, reject) => {
          request.onsuccess = () => {
            resolve(request.result);
          };
          request.onerror = () => {
            reject(request.error);
          };
        });
      } else {
        return {};
      }
    }
  
    // Funktion zum Verschieben der Quoten
    upgrade(version) {
      const db = window.indexedDB;
      if (version === this.version + 1) {
        const transaction = db.transaction('upgrade', 'readonly');
        const store = transaction.objectStore('data');
        for (const key in this.data) {
          store.add(this.sanitize(this.data[key]));
        }
        transaction.commit();
        return true;
      } else {
        return false;
      }
    }
  
    // Funktion zum Verschlüsseln von Daten
    encrypt(data) {
      return encrypt(data);
    }
  
    // Funktion zum Entschlüsseln von Daten
    decrypt(data) {
      return decrypt(data);
    }
  }