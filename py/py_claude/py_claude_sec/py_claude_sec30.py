from typing import Any, Dict, List, Optional, Type, TypeVar
import threading
from queue import Queue
import sqlite3
from abc import ABC, abstractmethod
import hashlib
from contextlib import contextmanager
import logging

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

T = TypeVar('T', bound='Model')

class ConnectionPool:
    def __init__(self, database: str, max_connections: int = 5):
        self.database = database
        self.max_connections = max_connections
        self.connections: Queue[sqlite3.Connection] = Queue(maxsize=max_connections)
        self.lock = threading.Lock()
        
        # Initialize connection pool
        for _ in range(max_connections):
            conn = sqlite3.connect(database, check_same_thread=False)
            conn.row_factory = sqlite3.Row
            self.connections.put(conn)
    
    @contextmanager
    def get_connection(self):
        connection = self.connections.get()
        try:
            yield connection
        finally:
            self.connections.put(connection)
    
    def close_all(self):
        while not self.connections.empty():
            conn = self.connections.get()
            conn.close()

class QueryBuilder:
    def __init__(self, model_class: Type[T]):
        self.model_class = model_class
        self.where_clauses: List[tuple] = []
        self.parameters: List[Any] = []
        self._limit: Optional[int] = None
        self._offset: Optional[int] = None
        
    def where(self, **kwargs) -> 'QueryBuilder':
        for key, value in kwargs.items():
            self.where_clauses.append((key, '=', '?'))
            self.parameters.append(value)
        return self
    
    def limit(self, limit: int) -> 'QueryBuilder':
        self._limit = limit
        return self
    
    def offset(self, offset: int) -> 'QueryBuilder':
        self._offset = offset
        return self
    
    def _build_query(self) -> tuple[str, List[Any]]:
        query = f"SELECT * FROM {self.model_class.__tablename__}"
        
        if self.where_clauses:
            conditions = ' AND '.join(
                f"{clause[0]} {clause[1]} {clause[2]}"
                for clause in self.where_clauses
            )
            query += f" WHERE {conditions}"
            
        if self._limit is not None:
            query += f" LIMIT {self._limit}"
            
        if self._offset is not None:
            query += f" OFFSET {self._offset}"
            
        return query, self.parameters

class LazyLoader:
    def __init__(self, query_builder: QueryBuilder):
        self.query_builder = query_builder
        self._results: Optional[List[T]] = None
    
    def _load(self) -> List[T]:
        if self._results is None:
            query, params = self.query_builder._build_query()
            with Database.get_instance().connection_pool.get_connection() as conn:
                cursor = conn.cursor()
                cursor.execute(query, params)
                rows = cursor.fetchall()
                self._results = [
                    self.query_builder.model_class._from_row(dict(row))
                    for row in rows
                ]
        return self._results
    
    def __iter__(self):
        return iter(self._load())
    
    def __getitem__(self, index):
        return self._load()[index]

class Database:
    _instance = None
    
    def __init__(self, database: str):
        self.connection_pool = ConnectionPool(database)
        
    @classmethod
    def initialize(cls, database: str):
        if cls._instance is None:
            cls._instance = cls(database)
    
    @classmethod
    def get_instance(cls) -> 'Database':
        if cls._instance is None:
            raise RuntimeError("Database not initialized")
        return cls._instance

class Model(ABC):
    __tablename__: str
    __sensitive_fields__: List[str] = []
    
    def __init__(self, **kwargs):
        for key, value in kwargs.items():
            setattr(self, key, value)
    
    @classmethod
    def _from_row(cls: Type[T], row: Dict[str, Any]) -> T:
        return cls(**row)
    
    @classmethod
    def query(cls: Type[T]) -> QueryBuilder:
        return QueryBuilder(cls)
    
    def save(self):
        fields = self.__dict__
        placeholders = ', '.join(['?' for _ in fields])
        columns = ', '.join(fields.keys())
        query = f"INSERT INTO {self.__tablename__} ({columns}) VALUES ({placeholders})"
        
        # Hash sensitive fields
        values = []
        for key, value in fields.items():
            if key in self.__sensitive_fields__:
                value = hashlib.sha256(str(value).encode()).hexdigest()
            values.append(value)
        
        with Database.get_instance().connection_pool.get_connection() as conn:
            try:
                cursor = conn.cursor()
                cursor.execute(query, values)
                conn.commit()
            except Exception as e:
                conn.rollback()
                logger.error(f"Error saving model: {e}")
                raise

class Transaction:
    def __init__(self):
        self.db = Database.get_instance()
    
    def __enter__(self):
        self.conn = self.db.connection_pool.get_connection().__enter__()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        if exc_type is None:
            self.conn.commit()
        else:
            self.conn.rollback()
        self.conn.__exit__(exc_type, exc_val, exc_tb)

# Example usage:
class User(Model):
    __tablename__ = 'users'
    __sensitive_fields__ = ['password', 'credit_card']
    
    def __init__(self, username: str, password: str, email: str, credit_card: str):
        super().__init__(
            username=username,
            password=password,
            email=email,
            credit_card=credit_card
        )

# Initialize database
Database.initialize('example.db')

# Example usage with transaction
def create_user(username: str, password: str, email: str, credit_card: str):
    with Transaction():
        user = User(
            username=username,
            password=password,
            email=email,
            credit_card=credit_card
        )
        user.save()

# Query example with lazy loading
def get_users_by_email(email: str) -> LazyLoader:
    return User.query().where(email=email).limit(10)._execute()

# Usage example:
if __name__ == "__main__":
    # Create a user
    create_user("john_doe", "secure_pass", "john@example.com", "1234-5678-9012-3456")
    
    # Query users (lazy loaded)
    users = get_users_by_email("john@example.com")
    
    # Data is only loaded when accessed
    for user in users:
        print(f"Username: {user.username}")