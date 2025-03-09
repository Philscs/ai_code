import os
import sqlite3
from typing import List

class Migration:
    def __init__(self, version: str, up: str, down: str):
        self.version = version
        self.up = up
        self.down = down

class MigrationManager:
    def __init__(self, db_path: str, migrations_path: str):
        self.db_path = db_path
        self.migrations_path = migrations_path
        self._init_db()

    def _init_db(self):
        """Initialize the migrations table if not exists."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS migrations (
                    version TEXT PRIMARY KEY
                )
            """)

    def _get_applied_migrations(self) -> List[str]:
        """Fetch all applied migrations from the database."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute("SELECT version FROM migrations")
            return [row[0] for row in cursor.fetchall()]

    def _load_migrations(self) -> List[Migration]:
        """Load migration files from the migrations path."""
        migrations = []
        for filename in sorted(os.listdir(self.migrations_path)):
            if filename.endswith(".sql"):
                version = filename.split("_")[0]
                with open(os.path.join(self.migrations_path, filename), "r") as file:
                    sections = file.read().split("-- DOWN")
                    up = sections[0].strip()
                    down = sections[1].strip() if len(sections) > 1 else ""
                    migrations.append(Migration(version, up, down))
        return migrations

    def migrate(self):
        """Apply pending migrations."""
        applied_migrations = set(self._get_applied_migrations())
        migrations = self._load_migrations()

        for migration in migrations:
            if migration.version not in applied_migrations:
                try:
                    with sqlite3.connect(self.db_path) as conn:
                        cursor = conn.cursor()
                        cursor.executescript(migration.up)
                        cursor.execute("INSERT INTO migrations (version) VALUES (?)", (migration.version,))
                        conn.commit()
                    print(f"Applied migration {migration.version}")
                except sqlite3.Error as e:
                    print(f"Error applying migration {migration.version}: {e}")
                    raise

    def rollback(self, target_version: str):
        """Rollback to a specific migration version."""
        applied_migrations = self._get_applied_migrations()
        migrations = {m.version: m for m in self._load_migrations()}

        for version in reversed(applied_migrations):
            if version == target_version:
                break

            if version in migrations:
                migration = migrations[version]
                try:
                    with sqlite3.connect(self.db_path) as conn:
                        cursor = conn.cursor()
                        cursor.executescript(migration.down)
                        cursor.execute("DELETE FROM migrations WHERE version = ?", (version,))
                        conn.commit()
                    print(f"Rolled back migration {migration.version}")
                except sqlite3.Error as e:
                    print(f"Error rolling back migration {migration.version}: {e}")
                    raise

    def check_conflicts(self):
        """Check for migration conflicts."""
        applied_migrations = set(self._get_applied_migrations())
        all_migrations = {m.version for m in self._load_migrations()}

        missing_migrations = applied_migrations - all_migrations
        if missing_migrations:
            print("Warning: Applied migrations not found in migration files:", missing_migrations)

        unapplied_migrations = all_migrations - applied_migrations
        if unapplied_migrations:
            print("Pending migrations:", unapplied_migrations)

# Example usage
if __name__ == "__main__":
    manager = MigrationManager(db_path="example.db", migrations_path="migrations")
    
    # Run migrations
    manager.migrate()

    # Check for conflicts
    manager.check_conflicts()

    # Rollback to a specific version
    # manager.rollback("001")
