import hashlib
import json
from cryptography.fernet import Fernet

class MessageBroker:
    def __init__(self, encryption_key):
        self.encryption_key = encryption_key
        self.subscribers = {}
        self.message_store = []
        self.dead_letter_queue = []

    def subscribe(self, topic, subscriber):
        if topic not in self.subscribers:
            self.subscribers[topic] = []
        self.subscribers[topic].append(subscriber)

    def publish(self, topic, message):
        encrypted_message = self.encrypt_message(message)
        self.message_store.append((topic, encrypted_message))
        self.notify_subscribers(topic, encrypted_message)

    def notify_subscribers(self, topic, message):
        if topic in self.subscribers:
            for subscriber in self.subscribers[topic]:
                decrypted_message = self.decrypt_message(message)
                if not self.is_message_replayed(decrypted_message):
                    subscriber.receive_message(topic, decrypted_message)
                else:
                    self.dead_letter_queue.append((topic, message))

    def encrypt_message(self, message):
        cipher_suite = Fernet(self.encryption_key)
        encrypted_message = cipher_suite.encrypt(json.dumps(message).encode())
        return encrypted_message

    def decrypt_message(self, encrypted_message):
        cipher_suite = Fernet(self.encryption_key)
        decrypted_message = cipher_suite.decrypt(encrypted_message).decode()
        return json.loads(decrypted_message)

    def is_message_replayed(self, message):
        message_hash = hashlib.sha256(json.dumps(message).encode()).hexdigest()
        if message_hash in self.message_store:
            return True
        self.message_store.append(message_hash)
        return False

class Subscriber:
    def __init__(self, name):
        self.name = name

    def receive_message(self, topic, message):
        print(f"{self.name} received message on topic '{topic}': {message}")

# Example usage
encryption_key = b'encryption_key'  # Replace with your encryption key
broker = MessageBroker(encryption_key)

subscriber1 = Subscriber("Subscriber 1")
subscriber2 = Subscriber("Subscriber 2")

broker.subscribe("topic1", subscriber1)
broker.subscribe("topic2", subscriber2)

broker.publish("topic1", {"data": "Message 1"})
broker.publish("topic2", {"data": "Message 2"})
