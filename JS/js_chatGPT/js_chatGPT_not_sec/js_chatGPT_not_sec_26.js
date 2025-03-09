class InMemoryDB {
    constructor() {
        this.data = {}; // Main storage
        this.indexes = {}; // Index storage
        this.transactions = []; // Transaction log
        this.pendingTransactions = []; // Current transaction buffer
        this.isTransactionActive = false; // Transaction status
    }

    // Create a new table
    createTable(tableName) {
        if (this.data[tableName]) {
            throw new Error(`Table ${tableName} already exists.`);
        }
        this.data[tableName] = [];
        this.indexes[tableName] = {};
    }

    // Insert a record
    insert(tableName, record) {
        if (!this.data[tableName]) {
            throw new Error(`Table ${tableName} does not exist.`);
        }
        this.data[tableName].push(record);
        this.updateIndexes(tableName, record);
        if (this.isTransactionActive) {
            this.pendingTransactions.push({ action: 'insert', tableName, record });
        }
    }

    // Query records
    query(tableName, query) {
        if (!this.data[tableName]) {
            throw new Error(`Table ${tableName} does not exist.`);
        }
        return this.data[tableName].filter(record => {
            return Object.entries(query).every(([key, value]) => record[key] === value);
        });
    }

    // Create an index
    createIndex(tableName, fieldName) {
        if (!this.data[tableName]) {
            throw new Error(`Table ${tableName} does not exist.`);
        }
        this.indexes[tableName][fieldName] = {};
        this.data[tableName].forEach(record => {
            const fieldValue = record[fieldName];
            if (!this.indexes[tableName][fieldName][fieldValue]) {
                this.indexes[tableName][fieldName][fieldValue] = [];
            }
            this.indexes[tableName][fieldName][fieldValue].push(record);
        });
    }

    // Update indexes when inserting data
    updateIndexes(tableName, record) {
        if (this.indexes[tableName]) {
            for (const field in this.indexes[tableName]) {
                const fieldValue = record[field];
                if (!this.indexes[tableName][field][fieldValue]) {
                    this.indexes[tableName][field][fieldValue] = [];
                }
                this.indexes[tableName][field][fieldValue].push(record);
            }
        }
    }

    // Transaction handling
    beginTransaction() {
        if (this.isTransactionActive) {
            throw new Error('Transaction already active.');
        }
        this.isTransactionActive = true;
        this.pendingTransactions = [];
    }

    commitTransaction() {
        if (!this.isTransactionActive) {
            throw new Error('No active transaction to commit.');
        }
        this.transactions.push(...this.pendingTransactions);
        this.isTransactionActive = false;
        this.pendingTransactions = [];
    }

    rollbackTransaction() {
        if (!this.isTransactionActive) {
            throw new Error('No active transaction to rollback.');
        }
        this.pendingTransactions.forEach(({ action, tableName, record }) => {
            if (action === 'insert') {
                this.data[tableName] = this.data[tableName].filter(r => r !== record);
            }
        });
        this.isTransactionActive = false;
        this.pendingTransactions = [];
    }

    // Simulate data persistence
    saveToDisk() {
        // Replace with actual file saving logic if needed
        return JSON.stringify(this.data);
    }

    loadFromDisk(serializedData) {
        // Replace with actual file loading logic if needed
        this.data = JSON.parse(serializedData);
    }
}

// Example usage
const db = new InMemoryDB();
db.createTable('users');
db.insert('users', { id: 1, name: 'Alice' });
db.insert('users', { id: 2, name: 'Bob' });

console.log(db.query('users', { name: 'Alice' }));

db.createIndex('users', 'id');
db.beginTransaction();
db.insert('users', { id: 3, name: 'Charlie' });
db.rollbackTransaction();
console.log(db.query('users', { id: 3 })); // Should be empty

const persistedData = db.saveToDisk();
console.log('Saved Data:', persistedData);
db.loadFromDisk(persistedData);
