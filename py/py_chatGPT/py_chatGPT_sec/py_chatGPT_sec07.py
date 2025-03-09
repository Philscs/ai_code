import sqlite3
import json
import os
import time
from cryptography.fernet import Fernet
from hashlib import sha256

# Initialisiere Schlüssel für die Verschlüsselung
ENCRYPTION_KEY = Fernet.generate_key()
cipher_suite = Fernet(ENCRYPTION_KEY)

class MessageBroker:
    def __init__(self, db_path="message_broker.db"):
        self.db_path = db_path
        self._init_db()

    def _init_db(self):
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS messages (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    topic TEXT NOT NULL,
                    message TEXT NOT NULL,
                    timestamp INTEGER NOT NULL,
                    hash TEXT NOT NULL UNIQUE
                )
            """)
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS dead_letter_queue (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    topic TEXT NOT NULL,
                    message TEXT NOT NULL,
                    timestamp INTEGER NOT NULL,
                    reason TEXT NOT NULL
                )
            """)
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS subscriptions (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    topic TEXT NOT NULL,
                    subscriber TEXT NOT NULL
                )
            """)

    def publish(self, topic, message):
        encrypted_message = cipher_suite.encrypt(message.encode())
        timestamp = int(time.time())
        message_hash = sha256((topic + message + str(timestamp)).encode()).hexdigest()

        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            try:
                cursor.execute("""
                    INSERT INTO messages (topic, message, timestamp, hash)
                    VALUES (?, ?, ?, ?)
                """, (topic, encrypted_message.decode(), timestamp, message_hash))
            except sqlite3.IntegrityError:
                # Nachricht mit diesem Hash existiert bereits (Replay-Angriff)
                self._add_to_dlq(topic, message, "Replay attack detected")

    def subscribe(self, topic, subscriber):
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute("""
                INSERT INTO subscriptions (topic, subscriber)
                VALUES (?, ?)
            """, (topic, subscriber))

    def fetch_messages(self, topic, subscriber):
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute("""
                SELECT message, timestamp FROM messages WHERE topic = ?
            """, (topic,))

            rows = cursor.fetchall()
            messages = []
            for encrypted_message, timestamp in rows:
                decrypted_message = cipher_suite.decrypt(encrypted_message.encode()).decode()
                messages.append({"message": decrypted_message, "timestamp": timestamp})

            return messages

    def _add_to_dlq(self, topic, message, reason):
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute("""
                INSERT INTO dead_letter_queue (topic, message, timestamp, reason)
                VALUES (?, ?, ?, ?)
            """, (topic, message, int(time.time()), reason))

    def fetch_dlq(self):
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM dead_letter_queue")
            return cursor.fetchall()

# Beispielverwendung
if __name__ == "__main__":
    broker = MessageBroker()

    # Subscriber anmelden
    broker.subscribe("news", "subscriber_1")

    # Nachricht veröffentlichen
    broker.publish("news", "Breaking News: Python is awesome!")

    # Nachrichten abrufen
    messages = broker.fetch_messages("news", "subscriber_1")
    for msg in messages:
        print(msg)

    # Dead-Letter-Queue überprüfen
    dlq = broker.fetch_dlq()
    if dlq:
        print("Dead-Letter-Queue:", dlq)
