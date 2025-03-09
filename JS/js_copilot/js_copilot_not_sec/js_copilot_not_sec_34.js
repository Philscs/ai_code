class MigrationManager {
  constructor() {
    this.migrations = [];
    this.currentVersion = 0;
  }

  addMigration(version, migrationFunction) {
    this.migrations.push({ version, migrationFunction });
  }

  async migrate(targetVersion) {
    if (targetVersion < this.currentVersion) {
      throw new Error('Cannot migrate to a lower version');
    }

    const migrationsToRun = this.migrations.filter(
      (migration) => migration.version > this.currentVersion && migration.version <= targetVersion
    );

    for (const migration of migrationsToRun) {
      await migration.migrationFunction();
      this.currentVersion = migration.version;
    }
  }

  async rollback(targetVersion) {
    if (targetVersion > this.currentVersion) {
      throw new Error('Cannot rollback to a higher version');
    }

    const migrationsToRollback = this.migrations.filter(
      (migration) => migration.version > targetVersion && migration.version <= this.currentVersion
    );

    for (const migration of migrationsToRollback.reverse()) {
      await migration.migrationFunction();
      this.currentVersion = migration.version - 1;
    }
  }
}

// Usage example
const migrationManager = new MigrationManager();

// Add migrations
migrationManager.addMigration(1, async () => {
  // Migration logic for version 1
});

migrationManager.addMigration(2, async () => {
  // Migration logic for version 2
});

// Migrate to a specific version
await migrationManager.migrate(2);

// Rollback to a specific version
await migrationManager.rollback(1);
