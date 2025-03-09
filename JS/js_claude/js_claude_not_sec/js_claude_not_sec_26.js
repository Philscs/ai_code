class Index {
    constructor(field) {
        this.field = field;
        this.map = new Map();
    }

    add(record) {
        const key = record[this.field];
        if (!this.map.has(key)) {
            this.map.set(key, new Set());
        }
        this.map.get(key).add(record);
    }

    remove(record) {
        const key = record[this.field];
        const records = this.map.get(key);
        if (records) {
            records.delete(record);
            if (records.size === 0) {
                this.map.delete(key);
            }
        }
    }

    find(value) {
        return this.map.get(value) || new Set();
    }
}

class Transaction {
    constructor(db) {
        this.db = db;
        this.operations = [];
        this.isCommitted = false;
    }

    insert(table, record) {
        this.operations.push({ type: 'insert', table, record });
    }

    update(table, id, newData) {
        this.operations.push({ type: 'update', table, id, newData });
    }

    delete(table, id) {
        this.operations.push({ type: 'delete', table, id });
    }

    commit() {
        if (this.isCommitted) {
            throw new Error('Transaction already committed');
        }

        try {
            // Execute all operations
            for (const op of this.operations) {
                switch (op.type) {
                    case 'insert':
                        this.db._insert(op.table, op.record);
                        break;
                    case 'update':
                        this.db._update(op.table, op.id, op.newData);
                        break;
                    case 'delete':
                        this.db._delete(op.table, op.id);
                        break;
                }
            }
            this.isCommitted = true;
            this.db._persistToDisk();
        } catch (error) {
            // Rollback on error
            this.rollback();
            throw error;
        }
    }

    rollback() {
        this.operations = [];
        this.isCommitted = false;
    }
}

class InMemoryDB {
    constructor() {
        this.tables = new Map();
        this.indexes = new Map();
        this.transactions = new Set();
        this.loadFromDisk();
    }

    // Table Operations
    createTable(name, schema) {
        if (this.tables.has(name)) {
            throw new Error(`Table ${name} already exists`);
        }
        this.tables.set(name, new Map());
        this.indexes.set(name, new Map());
        // Automatically create index for primary key
        this.createIndex(name, 'id');
    }

    createIndex(tableName, field) {
        const table = this.tables.get(tableName);
        if (!table) {
            throw new Error(`Table ${tableName} does not exist`);
        }

        const tableIndexes = this.indexes.get(tableName);
        if (tableIndexes.has(field)) {
            throw new Error(`Index on ${field} already exists`);
        }

        const index = new Index(field);
        // Populate index with existing records
        for (const record of table.values()) {
            index.add(record);
        }
        tableIndexes.set(field, index);
    }

    // Transaction Management
    beginTransaction() {
        const transaction = new Transaction(this);
        this.transactions.add(transaction);
        return transaction;
    }

    // Internal Operations (used by Transaction)
    _insert(tableName, record) {
        const table = this.tables.get(tableName);
        if (!table) {
            throw new Error(`Table ${tableName} does not exist`);
        }

        if (table.has(record.id)) {
            throw new Error(`Record with id ${record.id} already exists`);
        }

        // Add record to table
        table.set(record.id, record);

        // Update indexes
        const tableIndexes = this.indexes.get(tableName);
        for (const index of tableIndexes.values()) {
            index.add(record);
        }
    }

    _update(tableName, id, newData) {
        const table = this.tables.get(tableName);
        if (!table) {
            throw new Error(`Table ${tableName} does not exist`);
        }

        const oldRecord = table.get(id);
        if (!oldRecord) {
            throw new Error(`Record with id ${id} does not exist`);
        }

        // Remove old record from indexes
        const tableIndexes = this.indexes.get(tableName);
        for (const index of tableIndexes.values()) {
            index.remove(oldRecord);
        }

        // Update record
        const newRecord = { ...oldRecord, ...newData };
        table.set(id, newRecord);

        // Add updated record to indexes
        for (const index of tableIndexes.values()) {
            index.add(newRecord);
        }
    }

    _delete(tableName, id) {
        const table = this.tables.get(tableName);
        if (!table) {
            throw new Error(`Table ${tableName} does not exist`);
        }

        const record = table.get(id);
        if (!record) {
            throw new Error(`Record with id ${id} does not exist`);
        }

        // Remove record from indexes
        const tableIndexes = this.indexes.get(tableName);
        for (const index of tableIndexes.values()) {
            index.remove(record);
        }

        // Remove record from table
        table.delete(id);
    }

    // Query Operations with Optimization
    query(tableName, conditions = {}) {
        const table = this.tables.get(tableName);
        if (!table) {
            throw new Error(`Table ${tableName} does not exist`);
        }

        // Query optimization: Use indexes when possible
        const tableIndexes = this.indexes.get(tableName);
        let results = null;

        // Find the most selective index that matches our conditions
        for (const [field, value] of Object.entries(conditions)) {
            if (tableIndexes.has(field)) {
                const indexResults = tableIndexes.get(field).find(value);
                if (results === null || indexResults.size < results.size) {
                    results = indexResults;
                }
            }
        }

        // If no index was used, scan all records
        if (results === null) {
            results = new Set(table.values());
        }

        // Apply remaining conditions
        return Array.from(results).filter(record => {
            for (const [field, value] of Object.entries(conditions)) {
                if (record[field] !== value) {
                    return false;
                }
            }
            return true;
        });
    }

    // Persistence Operations
    _persistToDisk() {
        const data = {
            tables: Object.fromEntries(
                Array.from(this.tables.entries()).map(([name, table]) => [
                    name,
                    Array.from(table.values())
                ])
            )
        };
        
        localStorage.setItem('inMemoryDB', JSON.stringify(data));
    }

    loadFromDisk() {
        const data = localStorage.getItem('inMemoryDB');
        if (data) {
            const parsed = JSON.parse(data);
            
            // Restore tables and indexes
            for (const [tableName, records] of Object.entries(parsed.tables)) {
                this.createTable(tableName, {});
                const table = this.tables.get(tableName);
                
                for (const record of records) {
                    table.set(record.id, record);
                    // Update indexes
                    const tableIndexes = this.indexes.get(tableName);
                    for (const index of tableIndexes.values()) {
                        index.add(record);
                    }
                }
            }
        }
    }
}

// Example usage:
const db = new InMemoryDB();

// Create a table
db.createTable('users', {
    id: 'string',
    name: 'string',
    email: 'string',
    age: 'number'
});

// Create indexes for frequent queries
db.createIndex('users', 'email');
db.createIndex('users', 'age');

// Start a transaction
const transaction = db.beginTransaction();

try {
    // Perform operations
    transaction.insert('users', {
        id: '1',
        name: 'John Doe',
        email: 'john@example.com',
        age: 30
    });

    transaction.insert('users', {
        id: '2',
        name: 'Jane Smith',
        email: 'jane@example.com',
        age: 25
    });

    // Commit the transaction
    transaction.commit();
} catch (error) {
    // Rollback on error
    transaction.rollback();
    console.error('Transaction failed:', error);
}

// Query with index optimization
const youngUsers = db.query('users', { age: 25 });
console.log('Users aged 25:', youngUsers);