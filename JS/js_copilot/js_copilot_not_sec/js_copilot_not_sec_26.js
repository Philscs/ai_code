class InMemoryDatabaseManager {
  constructor() {
    this.data = {};
    this.indexes = {};
    this.transactions = [];
  }

  // Indexing
  createIndex(collection, field) {
    if (!this.indexes[collection]) {
      this.indexes[collection] = {};
    }
    this.indexes[collection][field] = new Map();
    for (const id in this.data[collection]) {
      const value = this.data[collection][id][field];
      if (value) {
        if (!this.indexes[collection][field].has(value)) {
          this.indexes[collection][field].set(value, new Set());
        }
        this.indexes[collection][field].get(value).add(id);
      }
    }
  }

  // Transactions
  startTransaction() {
    this.transactions.push({});
  }

  commitTransaction() {
    if (this.transactions.length === 0) {
      throw new Error('No active transaction');
    }
    const transaction = this.transactions.pop();
    for (const collection in transaction) {
      for (const id in transaction[collection]) {
        this.data[collection][id] = transaction[collection][id];
      }
    }
  }

  rollbackTransaction() {
    if (this.transactions.length === 0) {
      throw new Error('No active transaction');
    }
    this.transactions.pop();
  }

  // Query Optimization
  query(collection, conditions) {
    const ids = new Set(Object.keys(this.data[collection]));
    for (const field in conditions) {
      if (this.indexes[collection] && this.indexes[collection][field]) {
        const value = conditions[field];
        if (this.indexes[collection][field].has(value)) {
          const matchingIds = this.indexes[collection][field].get(value);
          ids.forEach((id) => {
            if (!matchingIds.has(id)) {
              ids.delete(id);
            }
          });
        } else {
          return [];
        }
      } else {
        ids.forEach((id) => {
          if (this.data[collection][id][field] !== conditions[field]) {
            ids.delete(id);
          }
        });
      }
    }
    return Array.from(ids).map((id) => this.data[collection][id]);
  }

  // ACID Compliance
  insert(collection, document) {
    if (!this.data[collection]) {
      this.data[collection] = {};
    }
    const id = Math.random().toString(36).substr(2, 9);
    this.data[collection][id] = document;
    return id;
  }

  update(collection, id, updates) {
    if (!this.data[collection] || !this.data[collection][id]) {
      throw new Error('Document not found');
    }
    const document = this.data[collection][id];
    const updatedDocument = { ...document, ...updates };
    if (this.transactions.length > 0) {
      this.transactions[this.transactions.length - 1][collection][id] = updatedDocument;
    } else {
      this.data[collection][id] = updatedDocument;
    }
  }

  delete(collection, id) {
    if (!this.data[collection] || !this.data[collection][id]) {
      throw new Error('Document not found');
    }
    if (this.transactions.length > 0) {
      delete this.transactions[this.transactions.length - 1][collection][id];
    } else {
      delete this.data[collection][id];
    }
  }

  // Data Persistence
  saveToFile(filepath) {
    const fs = require('fs');
    const dataToSave = JSON.stringify(this.data);
    fs.writeFileSync(filepath, dataToSave);
  }

  loadFromFile(filepath) {
    const fs = require('fs');
    const data = fs.readFileSync(filepath, 'utf8');
    this.data = JSON.parse(data);
  }
}

// Usage example
const db = new InMemoryDatabaseManager();
db.insert('users', { id: 1, name: 'John Doe', age: 30 });
db.insert('users', { id: 2, name: 'Jane Smith', age: 25 });
db.createIndex('users', 'name');
const result = db.query('users', { name: 'John Doe' });
console.log(result);
