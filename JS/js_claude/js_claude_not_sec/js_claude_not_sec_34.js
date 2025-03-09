class Migration {
    constructor(version, description) {
        this.version = version;
        this.description = description;
        this.dependencies = [];
        this.upFn = null;
        this.downFn = null;
        this.testFn = null;
    }

    up(fn) {
        this.upFn = fn;
        return this;
    }

    down(fn) {
        this.downFn = fn;
        return this;
    }

    test(fn) {
        this.testFn = fn;
        return this;
    }

    dependsOn(...versions) {
        this.dependencies.push(...versions);
        return this;
    }
}

class MigrationManager {
    constructor(database) {
        this.database = database;
        this.migrations = new Map();
        this.appliedMigrations = new Set();
    }

    // Fügt eine neue Migration hinzu
    addMigration(migration) {
        if (this.migrations.has(migration.version)) {
            throw new Error(`Migration version ${migration.version} already exists`);
        }
        this.migrations.set(migration.version, migration);
    }

    // Lädt den aktuellen Migrationsstatus aus der Datenbank
    async loadState() {
        try {
            const result = await this.database.query(
                "SELECT version FROM schema_migrations ORDER BY version"
            );
            this.appliedMigrations = new Set(result.rows.map(row => row.version));
        } catch (error) {
            // Erstelle die schema_migrations Tabelle, falls sie nicht existiert
            await this.database.query(`
                CREATE TABLE IF NOT EXISTS schema_migrations (
                    version VARCHAR(255) PRIMARY KEY,
                    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            `);
        }
    }

    // Berechnet die Migrationsreihenfolge unter Berücksichtigung von Abhängigkeiten
    _resolveDependencies(targetVersion = null) {
        const visited = new Set();
        const resolved = [];

        const visit = (version) => {
            if (visited.has(version)) return;
            visited.add(version);

            const migration = this.migrations.get(version);
            if (!migration) {
                throw new Error(`Migration ${version} not found`);
            }

            for (const dep of migration.dependencies) {
                visit(dep);
            }

            resolved.push(version);
        };

        // Wenn keine targetVersion angegeben ist, alle Migrationen auflösen
        const versionsToResolve = targetVersion 
            ? [targetVersion]
            : Array.from(this.migrations.keys());

        for (const version of versionsToResolve) {
            visit(version);
        }

        return resolved;
    }

    // Führt die Datentransformation und Tests durch
    async _executeMigration(migration, direction = 'up') {
        const fn = direction === 'up' ? migration.upFn : migration.downFn;
        
        if (!fn) {
            throw new Error(`No ${direction} function defined for migration ${migration.version}`);
        }

        // Starte eine Transaktion
        await this.database.query('BEGIN');

        try {
            // Führe die Migration aus
            await fn(this.database);

            if (direction === 'up') {
                // Führe Tests aus, falls vorhanden
                if (migration.testFn) {
                    await migration.testFn(this.database);
                }

                // Aktualisiere den Migrationsstatus
                await this.database.query(
                    'INSERT INTO schema_migrations (version) VALUES ($1)',
                    [migration.version]
                );
                this.appliedMigrations.add(migration.version);
            } else {
                // Entferne die Migration aus dem Status
                await this.database.query(
                    'DELETE FROM schema_migrations WHERE version = $1',
                    [migration.version]
                );
                this.appliedMigrations.delete(migration.version);
            }

            // Commit der Transaktion
            await this.database.query('COMMIT');
        } catch (error) {
            // Rollback bei Fehler
            await this.database.query('ROLLBACK');
            throw error;
        }
    }

    // Migriert das Schema zur Zielversion
    async migrateTo(targetVersion = null) {
        await this.loadState();
        
        // Bestimme die Migrationsreihenfolge
        const resolved = this._resolveDependencies(targetVersion);
        
        // Führe die Migrationen aus
        for (const version of resolved) {
            if (!this.appliedMigrations.has(version)) {
                const migration = this.migrations.get(version);
                console.log(`Applying migration ${version}: ${migration.description}`);
                await this._executeMigration(migration, 'up');
            }
        }
    }

    // Führt einen Rollback zur angegebenen Version durch
    async rollbackTo(targetVersion) {
        await this.loadState();

        // Finde alle Migrationen, die zurückgerollt werden müssen
        const appliedVersions = Array.from(this.appliedMigrations)
            .sort((a, b) => b.localeCompare(a)); // Absteigend sortieren

        for (const version of appliedVersions) {
            if (version > targetVersion) {
                const migration = this.migrations.get(version);
                console.log(`Rolling back migration ${version}: ${migration.description}`);
                await this._executeMigration(migration, 'down');
            }
        }
    }
}

// Beispielverwendung:
const createMigrationManager = async (database) => {
    const manager = new MigrationManager(database);

    // Beispiel-Migration 1: Erstelle Users-Tabelle
    const migration1 = new Migration('001', 'Create users table')
        .up(async (db) => {
            await db.query(`
                CREATE TABLE users (
                    id SERIAL PRIMARY KEY,
                    email VARCHAR(255) UNIQUE NOT NULL,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            `);
        })
        .down(async (db) => {
            await db.query('DROP TABLE users');
        })
        .test(async (db) => {
            // Teste die Migration
            await db.query("INSERT INTO users (email) VALUES ('test@example.com')");
            const result = await db.query("SELECT * FROM users WHERE email = 'test@example.com'");
            if (result.rows.length !== 1) {
                throw new Error('Migration test failed');
            }
        });

    // Beispiel-Migration 2: Füge Spalte zur Users-Tabelle hinzu
    const migration2 = new Migration('002', 'Add name to users')
        .dependsOn('001')
        .up(async (db) => {
            await db.query('ALTER TABLE users ADD COLUMN name VARCHAR(255)');
        })
        .down(async (db) => {
            await db.query('ALTER TABLE users DROP COLUMN name');
        });

    manager.addMigration(migration1);
    manager.addMigration(migration2);

    return manager;
};

export { Migration, MigrationManager, createMigrationManager };