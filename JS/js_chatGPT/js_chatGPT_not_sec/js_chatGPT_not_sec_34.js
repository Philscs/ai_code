const fs = require('fs');
const path = require('path');
const sqlite3 = require('sqlite3').verbose();

class MigrationManager {
  constructor(databasePath, migrationsDir) {
    this.db = new sqlite3.Database(databasePath);
    this.migrationsDir = migrationsDir;
    this.initMigrationTable();
  }

  initMigrationTable() {
    const query = `CREATE TABLE IF NOT EXISTS migrations (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT UNIQUE,
      applied_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )`;
    this.db.run(query);
  }

  async getAppliedMigrations() {
    return new Promise((resolve, reject) => {
      this.db.all('SELECT name FROM migrations', (err, rows) => {
        if (err) reject(err);
        else resolve(rows.map(row => row.name));
      });
    });
  }

  async runMigration(fileName, direction) {
    const filePath = path.join(this.migrationsDir, fileName);
    const migration = require(filePath);

    if (direction === 'up') {
      await migration.up(this.db);
      this.db.run('INSERT INTO migrations (name) VALUES (?)', [fileName]);
    } else if (direction === 'down') {
      await migration.down(this.db);
      this.db.run('DELETE FROM migrations WHERE name = ?', [fileName]);
    }
  }

  async migrate() {
    const appliedMigrations = await this.getAppliedMigrations();
    const allMigrations = fs.readdirSync(this.migrationsDir).filter(file => file.endsWith('.js'));

    for (const migration of allMigrations) {
      if (!appliedMigrations.includes(migration)) {
        console.log(`Applying migration: ${migration}`);
        await this.runMigration(migration, 'up');
      }
    }
  }

  async rollback() {
    const appliedMigrations = await this.getAppliedMigrations();
    if (appliedMigrations.length === 0) {
      console.log('No migrations to rollback.');
      return;
    }

    const lastMigration = appliedMigrations[appliedMigrations.length - 1];
    console.log(`Rolling back migration: ${lastMigration}`);
    await this.runMigration(lastMigration, 'down');
  }
}

module.exports = MigrationManager;

// Example migration file: migrations/001_initial_schema.js
/**
module.exports = {
  up: async (db) => {
    await new Promise((resolve, reject) => {
      db.run(`CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)`, (err) => {
        if (err) reject(err);
        else resolve();
      });
    });
  },
  down: async (db) => {
    await new Promise((resolve, reject) => {
      db.run(`DROP TABLE users`, (err) => {
        if (err) reject(err);
        else resolve();
      });
    });
  }
};
*/
