import asyncio
import json
import sqlite3
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Set

from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding, rsa
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC

# Datenmodelle
@dataclass
class Message:
    id: int
    sender: str
    content: bytes  # Verschlüsselter Inhalt
    timestamp: datetime
    room_id: Optional[str] = None
    recipient: Optional[str] = None
    attachment: Optional[bytes] = None

@dataclass
class Room:
    id: str
    name: str
    members: Set[str]
    
class ChatServer:
    def __init__(self, host: str = "localhost", port: int = 8888):
        self.host = host
        self.port = port
        self.clients: Dict[str, "ClientConnection"] = {}
        self.rooms: Dict[str, Room] = {}
        self.offline_messages: Dict[str, List[Message]] = {}
        self.db = Database("chat.db")
        
    async def start(self):
        server = await asyncio.start_server(
            self.handle_client, self.host, self.port
        )
        print(f"Server läuft auf {self.host}:{self.port}")
        async with server:
            await server.serve_forever()
            
    async def handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        client = ClientConnection(reader, writer, self)
        await client.handle_connection()
        
    def broadcast_to_room(self, room_id: str, message: Message):
        room = self.rooms.get(room_id)
        if room:
            for username in room.members:
                if username in self.clients:
                    asyncio.create_task(
                        self.clients[username].send_message(message)
                    )
                else:
                    self.store_offline_message(username, message)

    def store_offline_message(self, username: str, message: Message):
        if username not in self.offline_messages:
            self.offline_messages[username] = []
        self.offline_messages[username].append(message)
        self.db.store_message(message)

class ClientConnection:
    def __init__(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter, server: ChatServer):
        self.reader = reader
        self.writer = writer
        self.server = server
        self.username: Optional[str] = None
        self.private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048
        )
        self.public_key = self.private_key.public_key()
        self.message_keys: Dict[str, bytes] = {}
        
    async def handle_connection(self):
        try:
            await self.authenticate()
            await self.handle_offline_messages()
            await self.message_loop()
        finally:
            if self.username:
                del self.server.clients[self.username]
            self.writer.close()
            await self.writer.wait_closed()
            
    async def authenticate(self):
        auth_data = await self.receive_json()
        self.username = auth_data["username"]
        # Hier würde normalerweise eine richtige Authentifizierung stattfinden
        self.server.clients[self.username] = self
        
    async def handle_offline_messages(self):
        if self.username in self.server.offline_messages:
            messages = self.server.offline_messages[self.username]
            for message in messages:
                await self.send_message(message)
            del self.server.offline_messages[self.username]
            
    async def message_loop(self):
        while True:
            try:
                data = await self.receive_json()
                message = self.create_message(data)
                
                if message.room_id:
                    self.server.broadcast_to_room(message.room_id, message)
                elif message.recipient:
                    await self.send_private_message(message)
                    
            except asyncio.CancelledError:
                break
                
    async def send_private_message(self, message: Message):
        recipient = self.server.clients.get(message.recipient)
        if recipient:
            await recipient.send_message(message)
        else:
            self.server.store_offline_message(message.recipient, message)
            
    def create_message(self, data: dict) -> Message:
        content = self.encrypt_message(data["content"])
        attachment = None
        if "attachment" in data:
            attachment = self.encrypt_file(data["attachment"])
            
        return Message(
            id=0,  # Die ID wird von der Datenbank vergeben
            sender=self.username,
            content=content,
            timestamp=datetime.now(),
            room_id=data.get("room_id"),
            recipient=data.get("recipient"),
            attachment=attachment
        )
        
    def encrypt_message(self, content: str) -> bytes:
        if isinstance(content, str):
            content = content.encode()
        
        # Für Raumnachrichten verwenden wir einen gemeinsamen Schlüssel
        room_key = self.message_keys.get(self.username)
        if not room_key:
            room_key = Fernet.generate_key()
            self.message_keys[self.username] = room_key
            
        f = Fernet(room_key)
        return f.encrypt(content)
        
    def encrypt_file(self, file_data: bytes) -> bytes:
        # Ähnlich wie encrypt_message, aber optimiert für größere Dateien
        key = Fernet.generate_key()
        f = Fernet(key)
        encrypted_data = f.encrypt(file_data)
        
        # Verschlüssele den Datei-Schlüssel mit dem öffentlichen Schlüssel des Empfängers
        encrypted_key = self.public_key.encrypt(
            key,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )
        
        # Kombiniere verschlüsselte Daten und Schlüssel
        return encrypted_key + encrypted_data
        
    async def receive_json(self) -> dict:
        data = await self.reader.readline()
        return json.loads(data.decode())
        
    async def send_message(self, message: Message):
        data = {
            "sender": message.sender,
            "content": message.content,
            "timestamp": message.timestamp.isoformat(),
            "room_id": message.room_id,
            "recipient": message.recipient,
            "attachment": message.attachment.hex() if message.attachment else None
        }
        await self.send_json(data)
        
    async def send_json(self, data: dict):
        message = json.dumps(data) + "\n"
        self.writer.write(message.encode())
        await self.writer.drain()

class Database:
    def __init__(self, db_path: str):
        self.conn = sqlite3.connect(db_path)
        self.create_tables()
        
    def create_tables(self):
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                sender TEXT NOT NULL,
                content BLOB NOT NULL,
                timestamp DATETIME NOT NULL,
                room_id TEXT,
                recipient TEXT,
                attachment BLOB
            )
        """)
        self.conn.commit()
        
    def store_message(self, message: Message):
        self.conn.execute("""
            INSERT INTO messages 
            (sender, content, timestamp, room_id, recipient, attachment)
            VALUES (?, ?, ?, ?, ?, ?)
        """, (
            message.sender,
            message.content,
            message.timestamp,
            message.room_id,
            message.recipient,
            message.attachment
        ))
        self.conn.commit()
        
    def get_offline_messages(self, username: str) -> List[Message]:
        cursor = self.conn.execute("""
            SELECT * FROM messages 
            WHERE recipient = ? OR room_id IN (
                SELECT room_id FROM room_members WHERE username = ?
            )
        """, (username, username))
        
        messages = []
        for row in cursor:
            messages.append(Message(
                id=row[0],
                sender=row[1],
                content=row[2],
                timestamp=datetime.fromisoformat(row[3]),
                room_id=row[4],
                recipient=row[5],
                attachment=row[6]
            ))
        return messages

# Beispiel für die Verwendung
async def main():
    server = ChatServer()
    await server.start()

if __name__ == "__main__":
    asyncio.run(main())