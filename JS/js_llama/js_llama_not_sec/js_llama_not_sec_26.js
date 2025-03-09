class IMDBM {
    constructor() {
      // In-Memory-Datenbank
      this.db = {};
      
      // Indexe
      this.indices = new Map();
    }
  
    /**
     * Erstellt einen neuen Table im Datenbank.
     * @param {string} tableName - Name des Tables
     */
    createTable(tableName) {
      if (!this.db[tableName]) {
        this.db[tableName] = [];
      }
    }
  
    /**
     * Fügt eine Spalte hinzu zu einem Table.
     * @param {string} tableName - Name des Tables
     * @param {string} columnName - Name der Spalte
     */
    addColumn(tableName, columnName) {
      if (this.db[tableName]) {
        this.db[tableName].push(columnName);
      }
    }
  
    /**
     * Erstellt ein Index für eine bestimmte Spalte.
     * @param {string} tableName - Name des Tables
     * @param {string} columnName - Name der Spalte
     */
    createIndex(tableName, columnName) {
      if (this.db[tableName] && this.indices.has(columnName)) {
        this.indices.set(columnName, tableName);
      }
    }
  
    /**
     * Durchführt eine Transaktion im Datenbank.
     * @param {function} callback - Funktion, die nach der Transaktion ausgeführt wird
     */
    transaction(callback) {
      try {
        // Starte Transaktion
        const startTime = Date.now();
        
        // Durchführe Aktionen in der Transaktion
        const result = callback(this.db);
        
        // Fertig mit Transaktion
        const endTime = Date.now();
        
        console.log(`Transaktion beendet. Dauer: ${endTime - startTime}ms`);
      } catch (error) {
        throw error;
      }
    }
  
    /**
     * Durchführt eine Query im Datenbank.
     * @param {string} query - SQL-Query
     */
    query(query) {
      // Optimieren der Query
      const optimizedQuery = this.optimizeQuery(query);
      
      // Durchführe Query
      return this.db[optimizedQuery.tableName].map(row => row[optimizedQuery.columnName]);
    }
  
    /**
     * Optimiert eine SQL-Query.
     * @param {string} query - SQL-Query
     */
    optimizeQuery(query) {
      // Hier können wir die Query optimieren, z.B. durch Indexierung
      return { tableName: 'users', columnName: 'name' };
    }
  
    /**
     * Speichert Daten im Datenbank.
     * @param {object} data - Daten, die gespeichert werden sollen
     */
    save(data) {
      const tableName = Object.keys(data)[0];
      this.db[tableName].push(data[tableName]);
    }
  }
  
  // Beispielhaftes Verwenden des IMDBM
  const imdbm = new IMDBM();
  
  imdbm.createTable('users');
  imdbm.addColumn('users', 'name');
  imdbm.addColumn('users', 'email');
  
  imdbm.save({ name: 'John Doe', email: 'john@example.com' });
  
  console.log(imdbm.query("SELECT * FROM users WHERE name = 'John Doe'"));
  