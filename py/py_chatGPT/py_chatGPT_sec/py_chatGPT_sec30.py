import sqlite3
from threading import Lock
from contextlib import contextmanager

class ConnectionPool:
    def __init__(self, database, pool_size=5):
        self._pool_size = pool_size
        self._database = database
        self._lock = Lock()
        self._connections = []
        for _ in range(pool_size):
            self._connections.append(sqlite3.connect(database, check_same_thread=False))

    def acquire(self):
        with self._lock:
            if self._connections:
                return self._connections.pop()
            else:
                raise RuntimeError("No available connections in the pool")

    def release(self, connection):
        with self._lock:
            self._connections.append(connection)

    def close_all(self):
        with self._lock:
            for conn in self._connections:
                conn.close()

class ORM:
    def __init__(self, database, pool_size=5):
        self.pool = ConnectionPool(database, pool_size)

    @contextmanager
    def get_connection(self):
        conn = self.pool.acquire()
        try:
            yield conn
        finally:
            self.pool.release(conn)

    @contextmanager
    def transaction(self):
        with self.get_connection() as conn:
            try:
                conn.execute("BEGIN TRANSACTION;")
                yield conn
                conn.execute("COMMIT;")
            except Exception as e:
                conn.execute("ROLLBACK;")
                raise e

    def execute(self, query, params=None):
        if params is None:
            params = {}
        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(query, params)
            return cursor

    def fetchall(self, query, params=None):
        if params is None:
            params = {}
        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(query, params)
            rows = cursor.fetchall()
            return rows

    def fetchone(self, query, params=None):
        if params is None:
            params = {}
        with self.get_connection() as conn:
            cursor = conn.cursor()
            cursor.execute(query, params)
            row = cursor.fetchone()
            return row

# Beispiel der Verwendung
if __name__ == "__main__":
    orm = ORM("example.db")

    # Tabelle erstellen
    with orm.transaction() as conn:
        conn.execute("""
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT NOT NULL UNIQUE
        );
        """)

    # Daten einfügen
    orm.execute("INSERT INTO users (name, email) VALUES (:name, :email)", {
        "name": "Alice",
        "email": "alice@example.com"
    })

    # Daten abfragen
    users = orm.fetchall("SELECT * FROM users")
    for user in users:
        print(user)

    orm.pool.close_all()
