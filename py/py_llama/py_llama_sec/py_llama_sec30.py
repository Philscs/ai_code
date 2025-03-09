import sqlite3
from contextlib import contextmanager
from typing import List, Dict

# Database-Connection-Pooling
class ConnectionPool:
    def __init__(self, db_name: str):
        self.db_name = db_name
        self.connections = []
        self.lock = False

    @contextmanager
    def connection(self):
        if not self.lock:
            with sqlite3.connect(self.db_name) as conn:
                self.connections.append(conn)
                try:
                    yield conn
                finally:
                    self.connections.pop()
        else:
            raise ValueError("Connection pool is locked")

    def acquire_connection(self):
        while not self.lock and self.connections:
            return self.connections.pop()

    def release_connection(self, conn):
        if conn in self.connections:
            self.connections.append(conn)

# Custom-ORM
class ORM:
    def __init__(self, db_name: str):
        self.db_name = db_name
        self.connection_pool = ConnectionPool(db_name)
        self.table_cache = {}

    @contextmanager
    def transaction(self):
        conn = self.connection_pool.acquire_connection()
        try:
            with conn:
                yield conn
        finally:
            self.connection_pool.release_connection(conn)

    def execute(self, query: str, params: Dict[str, any]):
        if '?' in query:
            raise ValueError("SQL-Injektion verhindert")
        with self.transaction():
            conn = self.connection_pool.acquire_connection()
            cursor = conn.cursor()
            cursor.execute(query, params)
            conn.commit()

    def select(self, table_name: str, columns: List[str], where_clause: str):
        if table_name not in self.table_cache:
            query = f"SELECT {columns} FROM {table_name}"
            with self.transaction():
                conn = self.connection_pool.acquire_connection()
                cursor = conn.cursor()
                cursor.execute(query)
                rows = cursor.fetchall()
                self.table_cache[table_name] = rows
        return self.table_cache[table_name]

    def insert(self, table_name: str, columns: List[str], values: Dict[str, any]):
        query = f"INSERT INTO {table_name} ({columns}) VALUES ({self._value_to_string(values)})"
        with self.transaction():
            conn = self.connection_pool.acquire_connection()
            cursor = conn.cursor()
            cursor.execute(query, tuple(self._values_to_tuple(values)))
            conn.commit()

    def update(self, table_name: str, columns: List[str], where_clause: str):
        query = f"UPDATE {table_name} SET {columns} WHERE {where_clause}"
        with self.transaction():
            conn = self.connection_pool.acquire_connection()
            cursor = conn.cursor()
            cursor.execute(query)
            conn.commit()

    def delete(self, table_name: str, where_clause: str):
        query = f"DELETE FROM {table_name} WHERE {where_clause}"
        with self.transaction():
            conn = self.connection_pool.acquire_connection()
            cursor = conn.cursor()
            cursor.execute(query)
            conn.commit()

# Helper-Methoden
    def _value_to_string(self, values: Dict[str, any]):
        return ", ".join(f"{k} = ?" for k in values.keys())

    def _values_to_tuple(self, values: Dict[str, any]):
        return tuple(values.values())