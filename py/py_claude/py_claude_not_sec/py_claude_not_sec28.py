from datetime import datetime
import hashlib
import json
from typing import Dict, List, Optional
import sqlite3

class Migration:
    def __init__(self, version: str, description: str):
        self.version = version
        self.description = description
        self.up_queries: List[str] = []
        self.down_queries: List[str] = []
        self.dependencies: List[str] = []
        self.checksum: Optional[str] = None
    
    def up(self, query: str) -> 'Migration':
        """Fügt eine Up-Migration hinzu"""
        self.up_queries.append(query)
        return self
    
    def down(self, query: str) -> 'Migration':
        """Fügt eine Down-Migration (Rollback) hinzu"""
        self.down_queries.append(query)
        return self
    
    def depends_on(self, version: str) -> 'Migration':
        """Definiert Abhängigkeiten zu anderen Migrationen"""
        self.dependencies.append(version)
        return self
    
    def calculate_checksum(self) -> str:
        """Berechnet Prüfsumme der Migration für Konflikterkennung"""
        content = json.dumps({
            'version': self.version,
            'description': self.description,
            'up_queries': self.up_queries,
            'down_queries': self.down_queries,
            'dependencies': self.dependencies
        }, sort_keys=True)
        return hashlib.sha256(content.encode()).hexdigest()

class MigrationManager:
    def __init__(self, db_path: str):
        self.db_path = db_path
        self.migrations: Dict[str, Migration] = {}
        self.init_migration_table()
    
    def init_migration_table(self):
        """Initialisiert die Migrations-Tracking-Tabelle"""
        with sqlite3.connect(self.db_path) as conn:
            conn.execute("""
                CREATE TABLE IF NOT EXISTS migrations (
                    version TEXT PRIMARY KEY,
                    description TEXT,
                    checksum TEXT,
                    applied_at TIMESTAMP,
                    dependencies TEXT
                )
            """)
    
    def register_migration(self, migration: Migration) -> None:
        """Registriert eine neue Migration"""
        migration.checksum = migration.calculate_checksum()
        self.migrations[migration.version] = migration
    
    def check_conflicts(self) -> List[str]:
        """Überprüft auf Konflikte zwischen lokalen und angewendeten Migrationen"""
        conflicts = []
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            for version, migration in self.migrations.items():
                cursor.execute(
                    "SELECT checksum FROM migrations WHERE version = ?",
                    (version,)
                )
                result = cursor.fetchone()
                if result and result[0] != migration.checksum:
                    conflicts.append(version)
        return conflicts
    
    def get_pending_migrations(self) -> List[Migration]:
        """Ermittelt ausstehende Migrationen unter Berücksichtigung der Abhängigkeiten"""
        applied = set()
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute("SELECT version FROM migrations")
            for (version,) in cursor.fetchall():
                applied.add(version)
        
        pending = []
        for version, migration in self.migrations.items():
            if version not in applied:
                # Prüfe, ob alle Abhängigkeiten erfüllt sind
                if all(dep in applied for dep in migration.dependencies):
                    pending.append(migration)
        
        return pending
    
    def migrate(self) -> None:
        """Führt ausstehende Migrationen aus"""
        conflicts = self.check_conflicts()
        if conflicts:
            raise ValueError(f"Konflikte gefunden in Migrationen: {conflicts}")
        
        pending = self.get_pending_migrations()
        with sqlite3.connect(self.db_path) as conn:
            for migration in pending:
                try:
                    # Führe Up-Queries aus
                    for query in migration.up_queries:
                        conn.execute(query)
                    
                    # Speichere Migration als angewendet
                    conn.execute(
                        """
                        INSERT INTO migrations 
                        (version, description, checksum, applied_at, dependencies)
                        VALUES (?, ?, ?, ?, ?)
                        """,
                        (
                            migration.version,
                            migration.description,
                            migration.checksum,
                            datetime.now().isoformat(),
                            json.dumps(migration.dependencies)
                        )
                    )
                    conn.commit()
                    print(f"Migration {migration.version} erfolgreich angewendet")
                except Exception as e:
                    conn.rollback()
                    raise RuntimeError(
                        f"Fehler bei Migration {migration.version}: {str(e)}"
                    )
    
    def rollback(self, target_version: Optional[str] = None) -> None:
        """
        Führt Rollback von Migrationen durch
        Wenn target_version None ist, wird nur die letzte Migration zurückgerollt
        """
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            
            if target_version is None:
                # Hole letzte angewendete Migration
                cursor.execute(
                    "SELECT version FROM migrations ORDER BY applied_at DESC LIMIT 1"
                )
                result = cursor.fetchone()
                if not result:
                    print("Keine Migrationen zum Zurückrollen vorhanden")
                    return
                target_version = result[0]
            
            # Hole alle Migrationen, die zurückgerollt werden sollen
            cursor.execute(
                "SELECT version FROM migrations WHERE applied_at >= (SELECT applied_at FROM migrations WHERE version = ?) ORDER BY applied_at DESC",
                (target_version,)
            )
            versions_to_rollback = [row[0] for row in cursor.fetchall()]
            
            for version in versions_to_rollback:
                migration = self.migrations.get(version)
                if not migration:
                    raise ValueError(
                        f"Migration {version} nicht im System registriert"
                    )
                
                try:
                    # Führe Down-Queries aus
                    for query in migration.down_queries:
                        conn.execute(query)
                    
                    # Entferne Migration aus der Tracking-Tabelle
                    conn.execute(
                        "DELETE FROM migrations WHERE version = ?",
                        (version,)
                    )
                    conn.commit()
                    print(f"Migration {version} erfolgreich zurückgerollt")
                except Exception as e:
                    conn.rollback()
                    raise RuntimeError(
                        f"Fehler beim Rollback von Migration {version}: {str(e)}"
                    )

# Beispiel für die Verwendung:
if __name__ == "__main__":
    # Initialisiere Manager
    manager = MigrationManager("example.db")
    
    # Erstelle einige Beispiel-Migrationen
    migration1 = Migration(
        "001", 
        "Erstelle Benutzer-Tabelle"
    ).up("""
        CREATE TABLE users (
            id INTEGER PRIMARY KEY,
            username TEXT NOT NULL,
            email TEXT NOT NULL
        )
    """).down("""
        DROP TABLE users
    """)
    
    migration2 = Migration(
        "002",
        "Füge Zeitstempel zu Benutzer-Tabelle hinzu"
    ).depends_on("001").up("""
        ALTER TABLE users
        ADD COLUMN created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    """).down("""
        CREATE TABLE users_temp AS 
        SELECT id, username, email FROM users;
        
        DROP TABLE users;
        
        CREATE TABLE users (
            id INTEGER PRIMARY KEY,
            username TEXT NOT NULL,
            email TEXT NOT NULL
        );
        
        INSERT INTO users SELECT * FROM users_temp;
        
        DROP TABLE users_temp;
    """)
    
    # Registriere Migrationen
    manager.register_migration(migration1)
    manager.register_migration(migration2)
    
    # Führe Migrationen aus
    try:
        manager.migrate()
    except Exception as e:
        print(f"Fehler beim Ausführen der Migrationen: {e}")
    
    # Führe Rollback durch
    try:
        manager.rollback("001")  # Rolle bis Version 001 zurück
    except Exception as e:
        print(f"Fehler beim Rollback: {e}")