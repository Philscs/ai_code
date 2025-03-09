import sqlite3
from typing import List, Dict

class Migration:
    def __init__(self, name: str, query: str):
        self.name = name
        self.query = query

class Database:
    def __init__(self, db_name: str):
        self.db_name = db_name
        self.conn = sqlite3.connect(db_name)
        self.migrations = {}

    def add_migration(self, migration: Migration):
        self.migrations[migration.name] = migration

    def apply_migration(self, name: str):
        if name not in self.migrations:
            raise ValueError(f"Migration '{name}' nicht gefunden.")
        
        migration = self.migrations[name]
        try:
            with self.conn:
                self.conn.execute(migration.query)
        except sqlite3.Error as e:
            print(f"Fehler bei Migration '{name}': {e}")
            return False
        return True

    def rollback_migration(self, name: str):
        if name not in self.migrations:
            raise ValueError(f"Migration '{name}' nicht gefunden.")
        
        migration = self.migrations[name]
        try:
            with self.conn:
                previous_query = f"SELECT sql FROM sqlite_master WHERE name='{migration.name}';"
                previous_sql = self.conn.execute(previous_query).fetchone()[0]
                if migration.query != previous_sql:
                    raise ValueError(f"Rollback-Funktionalität für Migration '{name}' nicht 
unterstützt.")
                
                self.conn.execute(migration.query)
            return True
        except sqlite3.Error as e:
            print(f"Fehler bei Rollback von Migration '{name}': {e}")
            return False

    def detect_conflicts(self, name: str):
        if name not in self.migrations:
            raise ValueError(f"Migration '{name}' nicht gefunden.")
        
        migration = self.migrations[name]
        try:
            with self.conn:
                previous_query = f"SELECT sql FROM sqlite_master WHERE name='{migration.name}';"
                previous_sql = self.conn.execute(previous_query).fetchone()[0]
                
                if migration.query != previous_sql:
                    return True
            return False
        except sqlite3.Error as e:
            print(f"Fehler bei Konflikt detection von Migration '{name}': {e}")
            return None

# Beispiel Einsatz
db = Database("test.db")

migration1 = Migration("create_table", "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL);")
db.add_migration(migration1)

migration2 = Migration("insert_data", "INSERT INTO users (name) VALUES ('John Doe');")
db.add_migration(migration2)

# Migrationen durchführen
print(db.apply_migration("create_table"))
print(db.apply_migration("insert_data"))

# Rollback
print(db.rollback_migration("insert_data"))

# Konflikt detection
print(db.detect_conflicts("insert_data"))  # True

# Daten wiederherstellen
db.apply_migration("insert_data")